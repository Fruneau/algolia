#include <cstring>
#include <cstdint>
#include <cstdlib>

#include <iostream>
#include <vector>

/* {{{ Utils */

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
static int parse_integer(const char *&str, T max, T &out)
{
    if (!str) {
        return -1;
    }

    errno = 0;
    char *end = NULL;
    auto val = strtoul(str, &end, 10);

    if (errno != 0 || !end || val > max) {
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

    int parse(const char *str, bool expect_full) {
#define CHECK_NEXT(sep)  do {                                                \
        if (*str != sep) {                                                   \
            if (*str == 0) {                                                 \
                return expect_full ? -1 : 0;                                 \
            }                                                                \
            return -1;                                                       \
        }                                                                    \
    } while (0)

        RETHROW(parse_integer<uint16_t>(str, 2020, year), "invalid year");
        CHECK_NEXT('-');
        RETHROW(parse_integer<uint16_t>(str, 12, month), "invalid month");
        CHECK_NEXT('-');
        /* XXX the parser is not strict here, it does not check the number of
         * day is compatible with the year-month couple. We could use strptime
         * instead of a hand-made parser.
         */
        RETHROW(parse_integer<uint8_t>(str, 31, day), "invalid day");
        CHECK_NEXT(' ');
        RETHROW(parse_integer<uint8_t>(str, 24, hour), "invalid hour");
        CHECK_NEXT(':');
        RETHROW(parse_integer<uint8_t>(str, 59, minute), "invalid minute");
        CHECK_NEXT(':');
        RETHROW(parse_integer<uint8_t>(str, 59, second), "invalid minute");
        return 0;

#undef CHECK_NEXT
    }
};

/* }}} */
/* {{{ Command line parsing */

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
};

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

static int parse_opt(int &argc, char *argv[], options_t &options)
{
    auto cmd = RETHROW_PN(next_arg(argc, argv), "command is missing");

    /* Parse the command */
    if (strcmp(cmd, "distinct") == 0) {
        options.command = options_t::DISTINCT;
    } else
    if (strcmp(cmd, "top") == 0) {
        options.command = options_t::TOP;

        auto count = RETHROW_PN(next_arg(argc, argv), "missing top count");

        RETHROW(parse_integer(count, UINT64_MAX, options.count),
                "malformed top count");
    } else
    if (strcmp(cmd, "help") == 0) {
        options.command = options_t::HELP;
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

            RETHROW(options.from.parse(date, false),
                    "malformed from date: " << date);
        } else
        if (strcmp(arg, "--to") == 0) {
            auto date = RETHROW_PN(next_arg(argc, argv),
                                   "missing to date parameter");

            RETHROW(options.to.parse(date, false),
                    "malformed to date: " << date);
        } else
        if (strcmp(arg, "--") == 0) {
            break;
        }

        options.files.push_back(arg);
    }

    while (auto arg = next_arg(argc, argv)) {
        options.files.push_back(arg);
    }

    return 0;
}

/* }}} */

int main(int argc, char *argv[])
{
    const char *cmdline = next_arg(argc, argv);
    options_t opts;

    if (parse_opt(argc, argv, opts) < 0) {
        print_help(cmdline);
        return -1;
    }

    switch (opts.command) {
      case options_t::HELP:
        print_help(cmdline);
        return 0;

      case options_t::DISTINCT:
        std::cerr << "unimplemented command\n";
        return -1;

      case options_t::TOP:
        std::cerr << "unimplemented command\n";
        return -1;
    }
}
