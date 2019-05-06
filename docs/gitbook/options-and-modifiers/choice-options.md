<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Choice Options
Sometimes you want an option to have a fixed set of possible values, such as
for an enum. You use opt.choice() to add legal choices, one at a time, to an
option.

Choices are similar to [feature switches](Feature%20Switches) but instead of
multiple boolean options populating a single variable it is a single
non-boolean option setting its variable to one of multiple values.

~~~ cpp
enum class State { go, wait, stop };

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & state = cli.opt<State>("streetlight", State::wait)
        .desc("Color of street light.").valueDesc("COLOR")
        .choice(State::go, "green", "Means go!")
        .choice(State::wait, "yellow", "Means wait, even if you're late.")
        .choice(State::stop, "red", "Means stop.");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    switch (*state) {
        case State::stop: cout << "STOP!"; break;
        case State::go: cout << "Go!"; break;
        case State::wait: cout << "Wait"; break;
    }
    return EX_OK;
}
~~~

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --streetlight=COLOR  Color of street light.
      green   Means go!
      yellow  Means wait, even if you're late. (default)
      red     Means stop.

  --help               Show this message and exit.

$ a.out
Wait
$ a.out --streetlight
Error: Option requires value: --streetlight
$ a.out --streetlight=purple
Error: Invalid "--streetlight" value: purple
Error: Must be "green", "red", or "yellow"
$ a.out --streetlight=green
Go!
~~~
