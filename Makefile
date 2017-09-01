CXXFLAGS = -std=gnu++14 -g -O3 -Wall -Wextra -Werror \
    -funsigned-char \
    -fno-strict-aliasing \
    -fwrapv \
    -Wall \
    -Wextra \
    -Werror \
    -Wno-error=deprecated-declarations \
    -Wno-gnu-designator \
    -Wno-return-type-c-linkage \
    -Wempty-body \
    -Wsizeof-array-argument \
    -Wparentheses \
    -Wlogical-not-parentheses \
    -Wno-extern-c-compat \
    -Wno-nullability-completeness \
    -Wno-shift-negative-value \
    -Wchar-subscripts \
    -Wundef \
    -Wshadow \
    -Wwrite-strings \
    -Wsign-compare \
    -Wunused \
    -Wno-unused-parameter \
    -Wuninitialized \
    -Winit-self \
    -Wpointer-arith \
    -Wredundant-decls \
    -Wformat-nonliteral \
    -Wno-format-y2k \
    -Wmissing-format-attribute \
    -fno-rtti \
    -fno-exceptions \
    -Wnon-virtual-dtor \
    -Woverloaded-virtual

all: query

query: query.cpp

