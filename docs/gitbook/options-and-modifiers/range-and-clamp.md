<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Range and Clamp
When you want to limit a value to be within a range (inclusive) you can use
opt.range() to error out or opt.clamp() to convert values outside the range to
be equal to the nearest of the two edges.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & count = cli.opt<int>("<count>").clamp(1, 10);
    auto & letter = cli.opt<char>("<letter>").range('a','z');
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << string(*count, *letter) << endl;
    return EX_OK;
}
~~~

~~~ console
$ a.out 1000 b
bbbbbbbbbb
$ a.out 1000 1
Error: Out of range 'letter' value [a - z]: 1
~~~
