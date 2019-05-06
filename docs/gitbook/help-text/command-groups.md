<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Command Groups
Command groups collect related commands together in the help text, in the same
way that option groups do with options.

There are two predefined command groups:

| Name | Sort | Title      | Description                               |
|------|------|------------|-------------------------------------------|
| ""   | ""   | "Commands" | Default command group                     |
| "~"  | "~"  | ""         | Footer group, default location for "help" |

To group commands you either use cli.cmdGroup() to set the group name or create
the command using cli.command() from the context of another command that is
already in the command group that you want for the new command.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;

    // move 1a into 'First' group after creation
    cli.command("1a").cmdGroup("First").cmdSortKey("1");
    // create 1b in current 'First' group.
    cli.command("1b");
    // create 2a and move it into 'Second'
    cli.command("2a").cmdGroup("Second").cmdSortKey("2");
    // create 3a and move to 'Third'
    cli.command("3a").cmdGroup("Third").cmdSortKey("3");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return EX_OK;
}
~~~
Let's see the command groupings...

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

First:
  1a
  1b

Second:
  2a

Third:
  3a

Options:
  --help     Show this message and exit.
~~~
