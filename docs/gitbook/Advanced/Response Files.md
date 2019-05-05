<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Response Files
A response file is a collection of frequently used or generated arguments
saved as text, often with a ".rsp" extension, that is substituted into the
command line when referenced.

What you write:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & words = cli.optVec<string>("[words]").desc("Things you say.");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Words:";
    for (auto & w : words)
        cout << " " << w;
    return EX_OK;
}
~~~
What happens later:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] [words...]
  words     Things you say.

Options:
  --help    Show this message and exit.

$ a.out a b
Words: a b
$ echo c >one.rsp
$ a.out a b @one.rsp d
Words: a b c d
~~~
Response files can be used multiple times and the arguments in them can be
broken into multiple lines:

~~~ console
$ echo d >one.rsp
$ echo e >>one.rsp
$ a.out x @one.rsp y @one.rsp
Words: x d e y d e
~~~
Response files also can be nested, when a response file contains a reference
to another response file the path is relative to the parent response file,
not to the working directory.

~~~ console
$ md rsp & cd rsp
$ echo one @more.rsp >one.rsp
$ echo two three >more.rsp
$ cd ..
$ a.out @rsp/one.rsp
Words: one two three
~~~

Recursive response files will fail, don't worry!
~~~ console
$ echo "@one.rsp" >one.rsp
$ a.out @one.rsp
Error: Recursive response file: one.rsp
~~~

While generally useful response file processing can be disabled via
cli.responseFiles(false).
