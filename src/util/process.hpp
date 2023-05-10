#pragma once
#include <array>
#include <cstdio>
#include <thread>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <wait.h>

#include "error.hpp"
#include "fd.hpp"
#include "thread.hpp"

#ifdef CUTIL_NS
namespace CUTIL_NS {
#endif

class Process {
  private:
    pid_t               pid    = 0;
    bool                joined = false;
    FileDescriptor      fds[3][2];
    std::FILE*          pipes[3] = {nullptr};
    std::string         outputs[2];
    std::thread         output_collector;
    EventFileDescriptor output_collector_event;

    auto collect_outputs() -> void;

  public:
    struct OpenParameters {
        const char*         shell       = nullptr;
        const char*         working_dir = nullptr;
        std::array<bool, 3> open_pipe   = {false};
    };

    struct Result {
        enum ExitReason {
            Exit,
            Signal,
        };
        struct ExitStatus {
            ExitReason reason;
            int        code;
        };

        ExitStatus  status;
        std::string out;
        std::string err;
    };

    auto join(const bool force = false) -> Result {
        dynamic_assert(!joined, "already joined");

        joined = true;

        if(force) {
            dynamic_assert(kill(pid, SIGKILL) != -1, "failed to kill process");
        }

        auto status = int();
        waitpid(pid, &status, 0);

        output_collector_event.notify();
        output_collector.join();

        for(auto i = 0; i < 3; i += 1) {
            fclose(pipes[i]);
        }
        const auto exitted = static_cast<bool>(WIFEXITED(status));
        return {{exitted ? Result::Exit : Result::Signal, exitted ? WEXITSTATUS(status) : WTERMSIG(status)}, std::move(outputs[0]), std::move(outputs[1])};
    }

    auto get_pid() const -> pid_t {
        return pid;
    }

    auto get_stdin() -> FileDescriptor& {
        return fds[0][1];
    }
    
    auto is_joinable() const -> bool {
        return !joined;
    }

    // argv.back() and env.back() must be a nullptr
    Process(const std::vector<const char*>& argv, const std::vector<const char*>& env = {}, const char* const workdir = nullptr) {
        dynamic_assert(!argv.empty());
        dynamic_assert(argv.back() == NULL);
        dynamic_assert(env.empty() || env.back() == NULL);

        for(auto i = 0; i < 3; i += 1) {
            auto fd = std::array<int, 2>();
            if(pipe(fd.data()) < 0) {
                return;
            }
            fds[i][0] = FileDescriptor(fd[0]);
            fds[i][1] = FileDescriptor(fd[1]);
        }

        pid = vfork();
        if(pid < 0) {
            panic("vfork() failed");
        } else if(pid != 0) {
            for(auto i = 0; i < 3; i += 1) {
                const auto fd = fds[i][i == 0 ? 1 : 0].as_handle();
                if(fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
                    return;
                }
                pipes[i] = fdopen(fd, i == 0 ? "w" : "r");
                fds[i][i == 0 ? 0 : 1].close();
            }
            output_collector = std::thread([this]() -> void {
                auto fds = std::array{pollfd{.fd = fileno(pipes[1]), .events = POLLIN}, pollfd{.fd = fileno(pipes[2]), .events = POLLIN}, pollfd{.fd = output_collector_event, .events = POLLIN}};
                while(true) {
                    for(const auto& fd : fds) {
                        if(poll(fds.data(), fds.size(), -1) == -1) {
                            return;
                        }

                        if(fd.revents & POLLHUP) {
                            // target process is exitted
                            return;
                        }
                        if(!(fd.revents & POLLIN)) {
                            continue;
                        }
                        if(fd.fd == output_collector_event) {
                            return;
                        }
                        auto       buf = std::array<char, 256>();
                        const auto len = read(fd.fd, buf.data(), 256);
                        if(len == -1) {
                            if(errno == EAGAIN) {
                                break;
                            } else {
                                return;
                            }
                        }
                        outputs[0].insert(outputs[0].end(), buf.begin(), buf.begin() + len);
                        if(len < 256) {
                            break;
                        }
                    }
                }
            });
            return;
        } else {
            for(auto i = 0; i < 3; i += 1) {
                dup2(fds[i][i == 0 ? 0 : 1].as_handle(), i);
                fds[i][0].close();
                fds[i][1].close();
            }
            if(workdir != nullptr) {
                if(chdir(workdir) == -1) {
                    _exit(errno);
                }
            }
            execve(argv[0], const_cast<char* const*>(argv.data()), env.empty() ? environ : const_cast<char* const*>(env.data()));
            _exit(0);
        }
    }

    ~Process() {
        if(is_joinable()) {
            join();
        }
    }
};

#ifdef CUTIL_NS
}
#endif
