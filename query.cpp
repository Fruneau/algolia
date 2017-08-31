#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>
#include <functional>

/* {{{ Utils */

#define SRETHROW(a)  ({                                                      \
        typeof(a) __a = (a);                                                 \
                                                                             \
        if (__a < 0) {                                                       \
            return __a;                                                      \
        }                                                                    \
        __a;                                                                 \
    })

#define SRETHROW_PN(a)  ({                                                   \
        typeof(*(a)) *__a = (a);                                             \
                                                                             \
        if (__a == NULL) {                                                   \
            return -1;                                                       \
        }                                                                    \
        __a;                                                                 \
    })

#define RETHROW(a, msg)  ({                                                  \
        typeof(a) __a = (a);                                                 \
                                                                             \
        if (__a < 0) {                                                       \
            std::cerr << msg << "\n";                                        \
            return __a;                                                      \
        }                                                                    \
        __a;                                                                 \
    })

#define RETHROW_PN(a, msg)  ({                                               \
        typeof(*(a)) *__a = (a);                                             \
                                                                             \
        if (__a == NULL) {                                                   \
            std::cerr << msg << "\n";                                        \
            return -1;                                                       \
        }                                                                    \
        __a;                                                                 \
    })

template <typename T>
static int parse_integer(const char *&str, T min, T max, T &out)
{
    if (!str) {
        return -1;
    }

    errno = 0;
    char *end = NULL;
    auto val = strtoul(str, &end, 10);

    if (errno != 0 || !end || val > max || val < min) {
        return -1;
    }
    out = val;
    str = end;
    return 0;
}

/* }}} */
/* {{{ Date */

union date_t {
    uint64_t val;
    struct {
        uint8_t second;
        uint8_t minute;
        uint8_t hour;
        uint8_t day;
        uint16_t month;
        uint16_t year;
    };

    int parse(const char *&str, bool expect_full) {
#define CHECK_NEXT(sep)  do {                                                \
        if (*str != sep) {                                                   \
            if (*str == 0) {                                                 \
                return expect_full ? -1 : 0;                                 \
            }                                                                \
            return -1;                                                       \
        }                                                                    \
        str++;                                                               \
    } while (0)

        SRETHROW(parse_integer<uint16_t>(str, 2000, 2020, year));
        CHECK_NEXT('-');
        SRETHROW(parse_integer<uint16_t>(str, 1, 12, month));
        CHECK_NEXT('-');
        /* XXX the parser is not strict here, it does not check the number of
         * day is compatible with the year-month couple. We could use strptime
         * instead of a hand-made parser.
         */
        SRETHROW(parse_integer<uint8_t>(str, 1, 31, day));
        CHECK_NEXT(' ');
        SRETHROW(parse_integer<uint8_t>(str, 0, 24, hour));
        CHECK_NEXT(':');
        SRETHROW(parse_integer<uint8_t>(str, 0, 59, minute));
        CHECK_NEXT(':');
        SRETHROW(parse_integer<uint8_t>(str, 0, 59, second));
        return 0;

#undef CHECK_NEXT
    }
};

/* }}} */
/* {{{ File parser */

static int parse_file(const char *filename, FILE *file,
                      date_t from, date_t to,
                      std::function<void(const char *)> cb)
{
    char *buf = NULL;
    size_t buf_capp = 0;
    bool found_line = false;
    bool skipped_line = false;

    while (getline(&buf, &buf_capp, file) >= 0) {
        size_t len = strlen(buf);
        date_t date;

        if (buf[len - 1] == '\n') {
            buf[--len] = '\0';
        }

        const char *line = buf;
        date.val = 0;
        if (date.parse(line, true) < 0) {
            /* skip malformed line */
            skipped_line = true;
            continue;
        }

        if (*line != '\t') {
            /* skip malformed line */
            skipped_line = true;
            continue;
        }

        line++;
        if (*line == '\0') {
            /* skip malformed line */
            skipped_line = true;
            continue;
        }

        found_line = true;
        if (date.val >= from.val && date.val <= to.val) {
            cb(line);
        }
    }
    free(buf);

    if (ferror(file)) {
        std::cerr << "error while reading " << filename << ": " << strerror(errno) << "\n";
        return -1;
    } else
    if (!found_line && skipped_line) {
        std::cerr << "no valid line found in " << filename << "\n";
        return -1;
    }
    return 0;
}

static int do_on_files(const std::vector<const char *> &files,
                       date_t from, date_t to,
                       std::function<void(const char *)> cb)
{
    if (files.size() == 0) {
        return parse_file("standard input", stdin, from, to, cb);
    }

    bool has_valid_file = 0;

    for (auto filename : files) {
        auto file = fopen(filename, "r");

        if (!file) {
            std::cerr << "cannot open file " << filename << strerror(errno) << "\n";
            continue;
        }

        if (parse_file(filename, file, from, to, cb) >= 0) {
            has_valid_file = true;
        }

        fclose(file);
    }

    if (!has_valid_file) {
        std::cerr << "no valid file found\n";
        return -1;
    }
    return 0;
}

/* }}} */
/* {{{ Command handling */

static int distinct(const std::vector<const char *> &files,
                    date_t from, date_t to)
{
    std::unordered_set<std::string> queries;
    auto cb = [&queries](const char *query) {
        queries.insert(query);
    };

    SRETHROW(do_on_files(files, from, to, cb));

    std::cout << queries.size() << " distinct queries\n";
    return 0;
}

/* }}} */
/* {{{ Command line parsing */

static const char *next_arg(int &argc, char **&argv)
{
    if (argc <= 0) {
        return NULL;
    }

    auto out = argv[0];
    argc--;
    argv++;
    return out;
}

struct options_t {
    enum {
        DISTINCT,
        TOP,
        HELP
    } command;
    uint64_t count;

    date_t from;
    date_t to;
    std::vector<const char *> files;

    options_t(): command(HELP), count(0) {
        /* By default, set from/to to the extremes of boundaries.
         *
         * If partial dates are provided as argument, only the most
         * significant bytes will be overridden.
         */
        from.val = 0;
        to.val = UINT64_MAX;
    };

    int parse(int &argc, char *argv[]) {
        auto cmd = RETHROW_PN(next_arg(argc, argv), "command is missing");

        /* Parse the command */
        if (strcmp(cmd, "distinct") == 0) {
            command = options_t::DISTINCT;
        } else
        if (strcmp(cmd, "top") == 0) {
            command = options_t::TOP;

            auto count = RETHROW_PN(next_arg(argc, argv), "missing top count");

            RETHROW(parse_integer<uint64_t>(count, 1, UINT64_MAX, this->count),
                    "malformed top count");
        } else
        if (strcmp(cmd, "help") == 0) {
            command = options_t::HELP;
            return 0;
        } else {
            std::cerr << "unsupported command: " << cmd << "\n";
            return -1;
        }

        /* Parse the options */
        while (auto arg = next_arg(argc, argv)) {
            if (strcmp(arg, "--from") == 0) {
                auto date = RETHROW_PN(next_arg(argc, argv),
                                       "missing from date parameter");

                RETHROW(from.parse(date, false),
                        "malformed from date: " << date);
                continue;
            } else
            if (strcmp(arg, "--to") == 0) {
                auto date = RETHROW_PN(next_arg(argc, argv),
                                       "missing to date parameter");

                RETHROW(to.parse(date, false),
                        "malformed to date: " << date);
                continue;
            } else
            if (strcmp(arg, "--") == 0) {
                break;
            }

            files.push_back(arg);
        }

        while (auto arg = next_arg(argc, argv)) {
            files.push_back(arg);
        }

        return 0;
    }
};

/* }}} */

static void print_help(const char *cmd)
{
    std::cerr << cmd << " <command> [--from <from date>] [--to <to date>] [--] [<file>...]\n"
                 "\n"
                 "Commands:\n"
                 "    distinct\n"
                 "        compute the number of distinct searches performed\n"
                 "        between <from date> and <to date>.\n"
                 "    top <n>\n"
                 "        identifies the top <n> searches performed between\n"
                 "         <from date> and <to date>.\n"
                 "    help\n"
                 "        displays this help.\n"
                 "\n"
                 "Options:\n"
                 "    --from <from date>\n"
                 "        inclusive lower limit of the range of interesting\n"
                 "        dates. If omitted, no lower limit is applied.\n"
                 "    --to <to date>\n"
                 "        inclusive upper limit of the range of interseting\n"
                 "        dates. If omitted, no upper limit is applied.\n"
                 "    <file>...\n"
                 "        the log files to parse. If omitted, the data is\n"
                 "        read from the standard input\n"
                 "\n"
                 "Date format:\n"
                 "    <YYYY>[-<MM>[-<DD>[ <hh>[-<mm>[-<ss>]]]]]\n";
}

int main(int argc, char *argv[])
{
    const char *cmdline = next_arg(argc, argv);
    options_t opts;

    if (opts.parse(argc, argv) < 0) {
        print_help(cmdline);
        return -1;
    }

    switch (opts.command) {
      case options_t::HELP:
        print_help(cmdline);
        return 0;

      case options_t::DISTINCT:
        return distinct(opts.files, opts.from, opts.to);

      case options_t::TOP:
        std::cerr << "unimplemented command\n";
        return -1;
    }
}
