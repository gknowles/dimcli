<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Flag Arguments
Many arguments are flags with no associated value, they just set an option
to a predefined value. This is the default when you create a option of type
bool. Normally flags set the option to true, but this can be changed in two
ways:

- make it an inverted bool, which will set it to false
  - explicitly using the "!" modifier
  - define a long name and use the implicitly created "no-" prefix version
- use opt.flagValue() to set the value, see
  [feature switches](Feature%20Switches).

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & shout = cli.opt<bool>("shout !whisper").desc("I can't hear you!");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    string prog = argv[0];
    if (*shout) {
        auto & f = use_facet<ctype<char>>(cout.getloc());
        f.toupper(prog.data(), prog.data() + prog.size());
        prog += "!!!!111";
    }
    cout << "I am " << prog;
    return EX_OK;
}
~~~
What you see:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --shout, --no-whisper / --no-shout, --whisper
            I can't hear you!

  --help    Show this message and exit.

$ a.out
I am a.out
$ a.out --shout
I am A.OUT!!!!111
$ a.out --no-whisper
I am A.OUT!!!!111
~~~
