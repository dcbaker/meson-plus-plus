# Just a bunch of random design notes to myself

## Differences from Meson

Meson acts like an interpreter, running each line sequentially. Meson++
doesn't, I want it to act more like a compiler, as the Meson DSL really is
determanistic, given the same inputs it will always get the same outputs,
therefore we can just think of what's happening as compiling from a source
language to text, with some optimization happening along the way.

The compilers/linkers are laid out differently using a toolchain,
additionally the raw linker and a linker driver are differentiated, ie, gcc +
ld.bd is differant that just ld.bfd directly.

## organization

AST -> HIR -> backend -> text

The HIR looks a lot like Meson's state machine, but without being a state
machine, it instead loops over the IR, performing some optimizations (At
least dead code elimination, and probably const+copy propegation and
folding), and performing lowering passes to convert to the high level
constructs such as `include_directories` into raw strings.

The backend levels will perform any additional obvious optimization, then
create the necissary targets, which can then be written out to text. There is
no requirement that a backend have it's own IR, but there should be the
option to do so.

Finally the backend emits a text form the IR, ninja, vscode, xcode, etc.
