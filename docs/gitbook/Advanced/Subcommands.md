<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Subcommands
Git style subcommands are created by either cli.command("cmd"), which changes
the cli objects context to the command, or with opt.command("cmd"), which
changes the command that option is for. Once the cli object context has been
changed it can than be used to add (description, footer, options, etc) to the
command. Exactly the same as when working with a simple command line. If you
pass an empty string to cli.command() or opt.command() it represents the top
level processing that takes place before a command has been found.

Options are processed on the top level up to the first positional. The first
positional is the command, and the rest of the arguments are processed in the
context of that command. Since the top level doesn't process positionals when
commands are present, it will assert in debug builds and ignore them in
release if positionals are defined.
~~~ cpp
static auto & yell = Dim::Cli().opt<bool>("yell").desc("Say it loud.");
static auto & color = Dim::Cli().opt<string>("color", "red")
    .command("apple")
    .desc("Change color of the apple.");

int apple(Dim::Cli & cli) {
    cout << "It's a " << *color << " apple" << (*yell ? "!!!" : ".");
    return EX_OK;
}

int orange(Dim::Cli & cli) {
    cout << "It's an orange" << (*yell ? "!!!" : ".");
    return EX_OK;
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.command("apple").desc("Show apple. No other fruit.").action(apple);
    cli.command("orange").desc("Show orange.").action(orange);
    cli.exec(cerr, argc, argv);
    return cli.exitCode();
}
~~~

The same thing could also be done with external variables:
~~~ cpp
static bool yell;
static string color;
...

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.opt(&yell, "yell").desc("Say it loud.");
    cli.opt(&color, "color", "red").command("apple")
        .desc("Change color of the apple.");
    ...
~~~

Or if there's some additional argument checks or setup you need to do, the
exec() call can be separate from parse():
~~~ cpp
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    // any additional validation...
    cli.exec(cerr);
    return cli.exitCode();
~~~

The end result at the console:
~~~ console
$ a.out
Error: No command given.
$ a.out --help
usage: a.out [OPTIONS] command [args...]

Commands:
  apple     Show apple.
  orange    Show orange.

Options:
  --yell    Say it loud.

  --help    Show this message and exit.

$ a.out apple
It's a red apple.
$ a.out apple --color=yellow
It's a yellow apple.
$ a.out orange
It's an orange.
$ a.out --yell orange
It's an orange!!!
~~~

In the commands list, only the first sentence of cli.desc() (up to the first
'.', '!', or '?' that's followed by a space) is shown, but in command specific
pages you see the whole thing:

~~~ console
$ a.out apple --help
usage: a.out apple [OPTIONS]

Show apple. No other fruit.

Options:
  --color=STRING  Change color of the apple. (default: red)

  --help          Show this message and exit.
~~~
