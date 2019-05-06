<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Version Option
Use cli.versionOpt() to add simple --version processing.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.versionOpt("1.0");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Hello world!" << endl;
    return EX_OK;
}
~~~

Is version 1.0 ready to ship?
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help     Show this message and exit.
  --version  Show version and exit.

$ a.out --version
a.out version 1.0
$ a.out
Hello world!
~~~
