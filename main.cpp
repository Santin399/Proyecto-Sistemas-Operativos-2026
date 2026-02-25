#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#include <unistd.h>     
#include <sys/wait.h>   
#include <signal.h>     

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::vector<std::string> split_tokens(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    std::string t;
    while (iss >> t) tokens.push_back(t);
    return tokens;
}

static std::vector<char*> to_argv(std::vector<std::string>& parts) {
    std::vector<char*> argv;
    argv.reserve(parts.size() + 1);
    for (auto &p : parts) argv.push_back(const_cast<char*>(p.c_str()));
    argv.push_back(nullptr);
    return argv;
}

static bool is_builtin(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return false;
    return tokens[0] == "cd" || tokens[0] == "pwd" || tokens[0] == "exit";
}

static int run_builtin(const std::vector<std::string>& tokens) {
    const std::string& cmd = tokens[0];

    if (cmd == "exit") {
        return 1; 
    }

    if (cmd == "pwd") {
        char buf[4096];
        if (getcwd(buf, sizeof(buf)) != nullptr) {
            std::cout << buf << "\n";
        } else {
            perror("pwd/getcwd");
        }
        return 0;
    }

    if (cmd == "cd") {
        const char* path = nullptr;

        if (tokens.size() >= 2) {
            path = tokens[1].c_str();
        } else {
            path = getenv("HOME");
            if (!path) path = "/";
        }

        if (chdir(path) != 0) {
            perror("cd/chdir");
        }
        return 0;
    }

    return 0;
}

static void exec_single(std::vector<std::string> tokens, bool background) {
    if (tokens.empty()) return;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_DFL);

        auto argv = to_argv(tokens);
        execvp(argv[0], argv.data());
        perror("execvp");
        _exit(127);
    } else {
        // Padre
        if (background) {
            std::cout << "[" << pid << "] running\n";
        } else {
            int status = 0;
            if (waitpid(pid, &status, 0) < 0) perror("waitpid");
        }
    }
}

int main() {
    signal(SIGINT, SIG_IGN);

    std::string line;
    while (true) {
        std::cout << "mini$ " << std::flush;

        if (!std::getline(std::cin, line)) break;
        line = trim(line);
        if (line.empty()) continue;

        auto tokens = split_tokens(line);

        bool background = false;
        if (!tokens.empty() && tokens.back() == "&") {
            background = true;
            tokens.pop_back();
            if (tokens.empty()) continue;
        }

        if (is_builtin(tokens)) {
            int shouldExit = run_builtin(tokens);
            if (shouldExit == 1) break;
            continue;
        }

        exec_single(tokens, background);
    }

    std::cout << "\nChao chao.\n";
    return 0;
}
