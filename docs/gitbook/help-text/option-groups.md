<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Option Groups
Option groups are used to collect related options together in the help text.
In addition to name, groups have a title and sort key that determine section
heading and the order groups are rendered. Groups are created on reference,
with the title and sort key equal to the name.

Additionally there are two predefined option groups:

| Name | Sort | Title     | Description                                 |
|------|------|-----------|--------------------------------------------|
| ""   | ""   | "Options" | Default group options are created          |
| "~"  | "~"  | ""        | Footer group, default location for "--help" and "--version" |

In order to generate the help text the visible options are collected into
groups, the groups are sorted by sort key and the options within each group
are sorted by name.

The group title followed by the options is then output for each group that
has options. A group without a title is still separate from the previous group
by a single blank line.

To group options you either use opt.group() to set the group name or create
the option using cli.opt\<T>() after changing the context with cli.group().

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.versionOpt("1.0");
    // move 1b into 'First' group after creation
    cli.opt<bool>("1b").group("First").desc("boolean 1b");
    // set context to 'First' group, update its key and add 1a directly to it
    cli.group("First").sortKey("a").title(
        "First has a really long title that wraps around to more than "
        "a single line, quite a lot of text for so few options"
    );
    cli.opt<bool>("1a");
    // add 2a to 'Second'
    cli.group("Second").sortKey("b").opt<bool>("2a");
    cli.group("Third").sortKey("c").opt<bool>("3a");
    // give the footer group a title
    cli.group("~").title("Internally Generated");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return EX_OK;
}
~~~
Let's see the groupings...

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

First has a really long title that wraps around to more than a single line,
quite a lot of text for so few options:
  --1a
  --1b       boolean 1b

Second:
  --2a

Third:
  --3a

Internally Generated:
  --help     Show this message and exit.
  --version  Show version and exit.
~~~
