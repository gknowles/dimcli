<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Help Subcommand
A simple help command can be added via cli.helpCmd(). Having a help command
allows users to run the more natural "a.out help command" to get help with a
subcommand instead of the more annoying "a.out command --help".

How to add it:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.helpCmd();
    cli.exec(cerr, argc, argv);
    return cli.exitCode();
}
~~~

Programs that only have a simple help command aren't very helpful, but it
should give you an idea. If you have more commands they will show up as you'd
expect.
~~~ console
$ a.out help
usage: a.out [OPTIONS] command [args...]

Commands:
  help      Show help for individual commands and exit.

Options:
  --help    Show this message and exit.

$ a.out help help
usage: a.out help [OPTIONS] [command]

Show help for individual commands and exit. If no command is given the list of
commands and general options are shown.
  command    Command to show help information about.

Options:
  -u, --usage / --no-usage  Only show condensed usage.

  --help                    Show this message and exit.

$ a.out help -u
usage: a.out [--help] command [args..]
$ a.out help help -u
usage: a.out help [-u, --usage] [--help] [command]
~~~
