#include <iostream>

#define RETHOW(a)  ({                                                        \
        typeof(a) __a = (a);                                                 \
                                                                             \
        if (__a < 0) {                                                       \
            return __a;                                                      \
        }                                                                    \
        __a;                                                                 \
    })

void print_help(void)
{
    std::cerr << "query <command> [--from <from date>] [--to <to date>] [<file>]\n"
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
                 "    <file>\n"
                 "        the log file to parse. If omitted, the data is\n"
                 "        read from the standard input\n"
                 "\n"
                 "Date format:\n"
                 "    <YYYY>[-<MM>[-<DD>[ <hh>[-<mm>[-<ss>]]]]]\n";
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    print_help();
    return 0;
}
