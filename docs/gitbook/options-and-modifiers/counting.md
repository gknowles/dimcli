<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Counting
In very rare circumstances, it might be useful to use repetition to increase
an integer. There is no special handling for it, but counting can be done
easily enough with a vector. This can be used for verbosity flags, for
instance:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & v = cli.optVec<bool>("v verbose");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Verbosity: " << v.size();
    return EX_OK;
}
~~~
And on the command line:

~~~ console
$ a.out -vvv
Verbosity: 3
~~~

This could also be done with a [parse action](Parse%20Actions), but that seems
like more work.
