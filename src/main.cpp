#include "load_balancer.hpp"
#include "util.hpp"
#include <crypt.h>
#include <fstream>
#include <functional>
#include <memory>
#include <mpi.h>
#include <omp.h>
#include <optional>
#include <string>
#include <tuple>

using std::size_t;
using std::string;

constexpr std::array<char, 26> letters = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                                          'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                                          's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

constexpr int MAX_LENGTH = 4;
constexpr int CHAR_PACK = 440;
constexpr int SALT_PACK = 441;
constexpr int HASH_PACK = 442;
constexpr int BUFFER_SIZE = 256;

auto break_pass_impl(const string prefix, const size_t length, const string& salt,
                     const string& encrypted_password) -> std::optional<string> {
    if (length) {

        const char* passwd = crypt(prefix.data(), salt.c_str());
        auto result =
            encrypted_password == passwd ? std::optional(string{prefix.data()}) : std::nullopt;

        if (result) return result;

        for (auto letter : letters)
            if (auto tmp = break_pass_impl(prefix + letter, length - 1, salt, encrypted_password);
                tmp)
                return tmp;

        return std::nullopt;
    }
    return std::nullopt;
}

auto break_password(const string& start, const size_t length, const string& salt,
                    const string& hash) -> std::optional<string> {
    return break_pass_impl(start, length, salt, hash);
}

auto balance_work(std::vector<std::vector<char>>& nodes_work, std::vector<char>& remaining_work)
    -> void {
    if (remaining_work.size() >= nodes_work[0].size()) {}
}

// command to run: sudo mpiexec -n 26 --oversubscribe --allow-run-as-root
// --mca opal_warn_on_missing_libcuda 0 PasswordCracker

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (rank == 0) {

        logln("Process Rank: {}", rank);
        logln("total procs: {}", nprocs);
        string username{};
        log("Enter username: ");
        std::cin >> username;

        const auto record = get_secret(username);
        record ? 0 : throw std::runtime_error("User not found");

        const auto [salt, hash] = record.value();

        // hash = crypt("sex", salt.c_str());

        logln("salt: {}\nhash: {}", salt, hash);

        load_balancer<decltype(letters.begin())> lb{letters.begin(), letters.end(),
                                                    (std::size_t)nprocs};

        const auto [nodes_work, remaining_work] = lb.divide_work(true);

        for (int i = 1; i <= nprocs - 1; i++) { // slave processes = nprocs - 1
            // Sending work chunk to workers
            MPI_Send(nodes_work[i - 1].data(), (int)nodes_work[i - 1].size(), MPI_CHAR, i,
                     CHAR_PACK, MPI_COMM_WORLD);
            // Sending Salt to other processes
            MPI_Send(salt.c_str(), BUFFER_SIZE, MPI_CHAR, i, SALT_PACK, MPI_COMM_WORLD);
            // Sending Hash to other processes
            MPI_Send(hash.c_str(), BUFFER_SIZE, MPI_CHAR, i, HASH_PACK, MPI_COMM_WORLD);
        }

        if (remaining_work) {
            logln("[ROOT PROCESS]: Working on {}",
                  std::string(remaining_work.value().begin(), remaining_work.value().end()));

            const auto& value = remaining_work.value();
            const auto chunk = string(value.begin(), value.end());

            for (auto ch : chunk) {
                if (auto result = break_password({ch}, MAX_LENGTH, salt, salt + hash); result) {
                    logln("[ROOT PROCESS]: PASSWORD FOUND: {}\n", result.value());
                    MPI_Abort(MPI_COMM_WORLD, EXIT_SUCCESS);
                }
            }

        } else
            logln("[ROOT PRICESS]: Work have been equally divided among slaves");
    } else {

        int rank, chunk_size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        // logln("Slave Rank: {}", rank);
        char saltbuf[BUFFER_SIZE], hashbuf[BUFFER_SIZE];
        MPI_Status status;

        // Probing for the incomming chunk size

        MPI_Probe(0, CHAR_PACK, MPI_COMM_WORLD, &status);

        MPI_Get_count(&status, MPI_CHAR, &chunk_size);

        std::unique_ptr<char[]> chunk(new char[chunk_size]{0});

        MPI_Recv(chunk.get(), chunk_size, MPI_CHAR, 0, CHAR_PACK, MPI_COMM_WORLD, &status);

        MPI_Recv(saltbuf, sizeof saltbuf, MPI_CHAR, 0, SALT_PACK, MPI_COMM_WORLD, &status);

        MPI_Recv(hashbuf, sizeof hashbuf, MPI_CHAR, 0, HASH_PACK, MPI_COMM_WORLD, &status);

        const string prefix{chunk.release(), (size_t)chunk_size};
        const string salt = saltbuf;
        const string hash = hashbuf;

        logln("[SLAVE {}]: Working on: {}", rank, prefix);

        for (auto ch : prefix) {
            if (auto result = break_password({ch}, MAX_LENGTH, salt, salt + hash); result) {
                logln("[SLAVE PROCESS: {}]: PASSWORD FOUND: {}\n", rank, result.value());
                MPI_Abort(MPI_COMM_WORLD, EXIT_SUCCESS);
            }
        }
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
