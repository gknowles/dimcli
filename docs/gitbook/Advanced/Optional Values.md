<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Optional Values
You use the '?' [flag](Option%20Names) on an argument name to indicate that
its value is optional. Only non-booleans can have optional values, booleans
are evaluated just on their presence or absence and don't otherwise have
values.

For a user to set a value on the command line when it is optional the value
must be connected (no space) to the argument name, otherwise it is interpreted
as not present and the arguments implicit value is used instead. If the name
is not present at all the variable is set to the default given in the
cli.opt\<T>() call.

By default the implicit value is T{}, but can be changed using
opt.implicitValue().

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & v1 = cli.opt<string>("?o ?optional", "default");
    auto & v2 = cli.opt<string>("?i ?with-implicit", "default");
    v2.implicitValue("implicit");
    auto & p = cli.opt<string>("[positional]", "default");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "v1 = " << *v1 << ", v2 = " << *v2 << ", p = " << *p;
    return EX_OK;
}
~~~
What happens:

~~~ console
$ a.out
v1 = default, v2 = default, p = default
$ a.out -oone -i two
v1 = one, v2 = implicit, p = two
$ a.out -o one -itwo
v1 =, v2 = two, p = one
$ a.out --optional=one --with-implicit two
v1 = one, v2 = implicit, p = two
$ a.out --optional one --with-implicit=two
v1 =, v2 = two, p = one
~~~
