<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Life After Parsing
If you are using external variables you just access them directly after using
cli.parse() to populate them.

If you use the proxy object returned from cli.opt\<T>() you can dereference it
like a smart pointer to get at the value. In addition, you can test whether
it was explicitly set, find the argument name that populated it, and get the
position in argv\[] it came from.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & name = cli.opt<string>("n name", "Unknown");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name) {
        cout << "Using the unknown name." << endl;
    } else {
        cout << "Name selected using " << name.from()
            << " from argv[" << name.pos() << "]" << endl;
    }
    cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
~~~
What it does:

~~~ console
$ a.out
Using the unknown name.
Hello Unknown!
$ a.out -n John
Name selected using -n
Hello John!
$ a.out --name Mary
Name selected using --name from argv[2]
Hello Mary!
~~~
