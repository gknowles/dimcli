<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Page Layout
The main help page, and the help pages for subcommands, are built the same
way and made up of the same six (not counting [option groups](Option%20Groups))
sections.

| Section | Changed by | Description |
|---------|------------|-------------|
| Header | cli.header() | Generally a one line synopsis of the purpose of the command. |
| Usage | cli.opt() | Generated text list the defined positional arguments. |
| Description | cli.desc() | Text describing how to use the command and what it does. Sometimes used instead of the positionals list. |
| Commands | cli.command(), cli.desc(), opt.command() | List of commands and first line of their description, included if there are any git style subcommands. |
| Positionals | cli.opt(), opt.desc() | List of positional arguments and their descriptions, omitted if none have descriptions. |
| Options | cli.opt(), opt.desc(), opt.valueDesc(), opt.defaultDesc(), opt.show() | List of named options and descriptions, included if there are any visible options. |
| Footer | cli.footer() | Shown at the end, often contains references to further information. |

Within text, consecutive spaces are collapsed and words are wrapped at 80
columns. Newlines should be reserved for paragraph breaks.

~~~ cpp
Dim::Cli cli;
cli.header("Heading before usage");
cli.desc("Desciption of what the command does, including any general "
    "discussion of the various aspects of its use.");
cli.opt<bool>("[positional]");
cli.opt<string>("option").valueDesc("OPT_VAL").desc("About this option.");
cli.opt<int>("p", 1).desc("Option p.");
cli.opt<int>("q", 2).desc("Option q.").defaultDesc("two, yes TWO!");
cli.opt<int>("r", 3).desc("Option r.").defaultDesc("");
cli.footer(
    "Footer at end, usually with where to find more info.\n"
    "- first reference\n"
    "- second reference\n"
);
~~~

In this example the positionals section is omitted because the positional
doesn't have a description.

~~~ console
$ a.out --help
Heading before usage
usage: a.out [OPTIONS] positional

Description of what the command does, including any general discussion of the
various aspects of its use.

Options:
  --option=OPT_VAL  About this options.
  -p NUM            Option p. (default: 1)
  -q NUM            Option q. (default: two, yes TWO!)
  -r NUM            Option r.

  --help            Show this message and exit.

Footer at end, usually with where to find more info.
- first reference
- second reference
~~~
