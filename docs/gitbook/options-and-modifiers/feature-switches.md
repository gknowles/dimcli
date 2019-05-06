<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Feature Switches
Using flag arguments, feature switches are implemented by creating multiple
options that reference the same external variable and marking them flag
values.

To set the default, pass in a value of true to the flagValue() function of
the option that should be the default.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    string fruit;
    // "~" is the default option group for --help, --version, etc. Give
    // it a title so it doesn't look like more fruit.
    cli.group("~").title("Other options");
    cli.group("Type of fruit");
    cli.opt(&fruit, "o orange", "orange").desc("oranges").flagValue();
    cli.opt(&fruit, "a", "apple").desc("red fruit").flagValue(true);
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << fruit << " have a worm? No!";
    return EX_OK;
}
~~~
Which looks like:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Type of fruit:
  -a            red fruit (default)
  -o, --orange  oranges

Other options:
  --help        Show this message and exit.

$ a.out
Does the apple have a worm? No!
$ a.out -o
Does the orange have a worm? No!
~~~
You can use an inaccessible option (empty string for the keys) that doesn't
show up in the interface (or the help text) to set an explicit default.

~~~ cpp
cli.opt(&fruit, "o orange", "orange").desc("oranges").flagValue();
cli.opt(&fruit, "a", "apple").desc("red fruit").flagValue();
cli.opt(&fruit, "", "fruit").flagValue(true);
~~~
Now instead of an apple there's a generic fruit default.

~~~ console
$ a.out
Does the fruit have a worm? No!
~~~
