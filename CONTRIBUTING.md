# Contributing

Meson++ has reached the point where I'm happy to have contributions from external developers. Your contributions can range from bug fixes to new features, to major code refactors. Here are some guidelines to help everyone out (both me and you as a contributor)

## Code style

* Code should be writting in C++, making use of modern C++ (Currently C++17) features.
* "Optional" braces are not optional, all loops and if's should use braces, even if they're only one line.
* run clang-format. If clang-format is being not helpful (This happens) use comments to turn it off. Tabular data is a great example of clang-format not being helpful
* use const as much as possible

## Bug fixes

Bug fixes do not need an issue attached to them, but if there is an issue be sure to mark the patches as fixing them so they're automatically closed. Please search the issue tracker to be sure you've found all fixed issues.

## Major contributions and refactors

If there is an issue opened, please comment saying that you're working on it. I try to assign issues I'm working on to myself, and I will assign you to it if you're going to work on it. This helps us not step on each others toes.

If there is not an issue opened, you don't need to open one. You might consider asking about your idea before starting, in case someone else is working on it, or there is a reason that something is architected the way it is.
