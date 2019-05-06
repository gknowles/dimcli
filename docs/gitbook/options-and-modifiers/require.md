<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Require
A simple way to make sure an option is specified is to mark it required with
opt.require(). This adds an after action that fails if no explicit value was
set for the option.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & file = cli.opt<string>("file f").require();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Selected file: " << *file << endl;
    return EX_OK;
}
~~~

What you get:
~~~ console
$ a.out
Error: No value given for --file
$ a.out -ffile.txt
Selected file: file.txt
~~~

The error message references the first name in the list so if you flip it
around...
~~~ cpp
auto & file = cli.opt<string>("f file").require();
~~~

... it will complain about '-f' instead of '--file'.
~~~ console
$ a.out
Error: No value given for -f
~~~
