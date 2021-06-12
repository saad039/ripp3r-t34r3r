#include "util.hpp"
#include <crypt.h>
#include <fstream>
#include <functional>
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

constexpr int MAX_LENGTH = 2;
constexpr int CHAR_PACK = 440;
constexpr int SALT_PACK = 441;
constexpr int HASH_PACK = 442;
constexpr int BUFFER_SIZE = 1024;

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

// command to run: sudo mpiexec --allow-run-as-root --mca opal_warn_on_missing_libcuda 0
// PasswordCracker

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

        logln("salt: {}\nhash: {}", salt, hash);

        for (int i = 1; i < 26; i++) {
            // Send b,c,d,...,z to other processes
            MPI_Send(&letters[i], 1, MPI_CHAR, i, CHAR_PACK, MPI_COMM_WORLD);
            // Sending Salt to other processes
            MPI_Send(salt.c_str(), BUFFER_SIZE, MPI_CHAR, i, SALT_PACK, MPI_COMM_WORLD);
            // Sending Hash to other processes
            MPI_Send(hash.c_str(), BUFFER_SIZE, MPI_CHAR, i, HASH_PACK, MPI_COMM_WORLD);
        }
        // start processing a
        auto res = break_password({letters[0]}, MAX_LENGTH, salt, hash);

        // if any process finds the password, MPI_ABORT
        if (res) {
            logln("[Rank: {}]: Password: {}\n", rank, res.value());
            MPI_Abort(MPI_COMM_WORLD, EXIT_SUCCESS);
        } else
            logln("Not Found");
    } else {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        // logln("Slave Rank: {}", rank);
        char pref[BUFFER_SIZE], saltbuf[BUFFER_SIZE], hashbuf[BUFFER_SIZE];
        MPI_Status status;

        MPI_Recv(pref, sizeof pref, MPI_CHAR, 0, CHAR_PACK, MPI_COMM_WORLD, &status);
        MPI_Recv(saltbuf, sizeof saltbuf, MPI_CHAR, 0, SALT_PACK, MPI_COMM_WORLD, &status);
        MPI_Recv(hashbuf, sizeof hashbuf, MPI_CHAR, 0, HASH_PACK, MPI_COMM_WORLD, &status);

        const string prefix = pref;
        const string salt = saltbuf;
        const string hash = hashbuf;

        logln("[Rank {}]: Working on: {}", rank, prefix);

        auto res = break_password(prefix, MAX_LENGTH, salt, hash);
        if (res) {
            logln("[Rank: {}]: Password: {}\n", rank, res.value());
            MPI_Abort(MPI_COMM_WORLD, EXIT_SUCCESS);
        } else
            logln("[Rank: {}]: Not Found", rank);
    }

    MPI_Finalize();
    return EXIT_SUCCESS;
}
