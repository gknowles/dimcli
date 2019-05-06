<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Help Option
You can modify the implicitly create --help option. Use cli.helpOpt() to get
a reference and then go to town. The most likely thing would be to change the
description or option group, but since you get back an Opt\<T> you can use any
of the standard functions.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.helpOpt();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return EX_OK;
}
~~~

And when run...
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help    Show this message and exit.
~~~

It can be modified like any other bool option.
~~~ cpp
cli.helpOpt().desc("What you see is what you get.");
~~~
~~~ cpp
auto & help = cli.helpOpt();
help.desc("What you see is what you get.");
~~~

Either of which gets you this:
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help    What you see is what you get.
~~~

Another related command is cli.helpNoArgs(), which internally adds "--help" to
otherwise empty command lines.
~~~ cpp
cli.helpNoArgs();
cli.helpOpt().desc("What you see is what you get.");
~~~

Now all there is, is help:
~~~ console
$ a.out
usage: a.out [OPTIONS]

Options:
  --help    What you see is what you get.
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help    What you see is what you get.
~~~
