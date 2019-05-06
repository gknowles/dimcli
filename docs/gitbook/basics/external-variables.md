<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## External Variables
In addition to using the option proxies you can bind options directly to
predefined variables. This can be used to set a global flag, or populate a
struct that you access later.

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    bool worm;
    Dim::Cli cli;
    cli.opt(&worm, "w worm").desc("make it icky");
    auto & fruit = cli.opt<string>("fruit", "apple").desc("type of fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << *fruit << " have a worm? "
        << worm ? "Yes :(" : "No!";
    return EX_OK;
}
~~~
And what it looks like:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --fruit=STRING  type of fruit (default: apple)
  -w, --worm      make it icky

  --help          Show this message and exit.

$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out -w
Does the apple have a worm? Yes :(
~~~

You can also point multiple "options" at the same variable, as is common with
[feature switches](Feature%20Switches).
