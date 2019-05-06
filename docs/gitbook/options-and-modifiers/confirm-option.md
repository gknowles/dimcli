<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Confirm Option
There is a short cut for a "-y, --yes" option, called cli.confirmOpt(), that
only lets the program run if the option is set or the user responds with 'y'
or 'Y' when asked if they are sure. Otherwise it sets cli.exitCode() to EX_OK
and causes cli.parse() to return false.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.confirmOpt();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "HELLO!!!";
    return EX_OK;
}
~~~
Cover your ears...

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -y, --yes  Suppress prompting to allow execution.

  --help     Show this message and exit.

$ a.out -y
HELLO!!!
$ a.out
Are you sure? [y/N]: n
$ a.out
Are you sure? [y/N]: y
HELLO!!!
~~~
You can change the prompt:

~~~ cpp
cli.confirmOpt("Are loud noises okay?");
~~~
Now it asks:

~~~ console
$ a.out
Are loud noises okay? [y/N]: y
HELLO!!!
~~~
