<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Option Names
Names are passed in as a space separated list where the individual names look
like these:

| Type of name                        | Example      |
|-------------------------------------|--------------|
| short name (single character)       | f            |
| long name (more than one character) | file         |
| optional positional                 | \[file name] |
| required positional                 | \<file\>     |

Names for positionals (inside angled or square brackets) may contain spaces,
and all names may have modifier flags:

| Flag | Type   | Description                                                     |
| :--: |--------|-----------------------------------------------------------------|
| !    | prefix | for boolean values, when setting the value it is first inverted |
| ?    | prefix | for non-boolean named options, makes the value [optional](Optional%20Values) |
| .    | suffix | for long names, suppresses the implicit "no-" version           |

By default, long names for boolean values get a second "no-" version implicitly
created for them.

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.opt<string>("a apple [apple]").desc("apples are red");
    cli.opt<bool>("!o orange apricot.").desc("oranges are orange");
    cli.opt<string>("<pear>").desc("pears are yellow");
    cli.parse(cerr, argc, argv);
    return EX_OK;
}
~~~
Ends up looking like this (note: required positionals are **always** placed
before any optional ones):

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] pear [apple]
  pear      pears are yellow
  apple     apples are red

Options:
  -a, --apple=STRING          apples are red
  --apricot, --orange / -o, --no-orange
                              oranges are orange

  --help                      Show this message and exit.
~~~

When named options are added they replace any previous rule with the same
name, therefore this option declares '-n' an inverted bool:

~~~ cpp
cli.opt<bool>("n !n");
~~~
But with these it becomes '-n STRING', a string:

~~~ cpp
cli.opt<bool>("n !n");
cli.opt<string>("n");
~~~
