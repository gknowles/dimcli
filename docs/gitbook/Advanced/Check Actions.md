<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Check Actions
Check actions run for each value that is successfully parsed and are a good
place for additional work. For example, opt.range() and opt.clamp() are
implemented as check actions. Just like parse actions the callback is any
std::function compatible object that accepts references to cli, opt, and
string as parameters and returns bool.

An option can have any number of check actions and they are called in the
order they were added.

The function should:
- Check the options new value. Beware that options are process in the order
  they appear on the command line, so comparing with another option is
  usually better done in an [after action](After%20Actions).
- Call cli.badUsage() with an error message if there's a problem.
- Return false if the program should stop, otherwise true to let processing
  continue.

The opt is fully populated, so *opt, opt.from(), etc are all available.

Sample check action that rounds up to an even number of socks:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & socks = cli.opt<int>("socks")
        .desc("Number of socks, rounded up to even number.")
        .check([](auto & cli, auto & opt, auto & val) {
            *opt += *opt % 2;
            return true;
        });
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << *socks << " socks";
    if (*socks) cout << ", where are the people?";
    cout << endl;
    return EX_OK;
}
~~~

Let's... wash some socks?
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -socks=NUM  Number of socks, rounded up to even number.

  --help      Show this message and exit.

$ a.out
0 socks
$ a.out --socks 3
4 socks, where are the people?
~~~
