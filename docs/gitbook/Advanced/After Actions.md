<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## After Actions
After actions run after all arguments have been parsed. For example,
opt.prompt() and opt.require() are both implemented as after actions. Any
number of after actions can be added and will, for every (not just the
ones referenced by the command line!) registered option, be called in the
order they're added. They are called with the three parameters, like other
option actions, that are references to cli, opt, and the value string
respectively. However the value string is always empty(), so any information
about the value must come from the opt reference.

When using subcommands, only the after actions bound to the top level or the
selected command are executed. After actions on the options of all other
commands are, like the options themselves, ignored.

The function should:
- Do something interesting.
- Call cli.badUsage() and return false on error.
- Return true if processing should continue.

Action to make sure the high is not less than the low:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & low = cli.opt<int>("l").desc("Low value.");
    auto & high = cli.opt<int>("h")
        .desc("High value, must be greater than the low.")
        .after([&](auto & cli, auto & opt, auto &) {
            return (*opt >= *low)
                || cli.badUsage("High must not be less than the low.");
        });
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Range is from " << *low << " to " << *high << endl;
    return EX_OK;
}
~~~

Set the range:
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -h=NUM    High value, must be greater than the low.
  -l=NUM    Low value.

  --help    Show this message and exit.

$ a.out
Range is from 0 to 0
$ a.out -l1
High must not be less than the low.
$ a.out -h5 -l2
Range is from 2 to 5
~~~
