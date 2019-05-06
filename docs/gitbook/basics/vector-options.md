<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Vector Options
Allows for an unlimited (or specific) number of values to be returned in a
vector. Vector options are declared using cli.optVec() which binds to a
std::vector\<T>.

Example:

~~~ cpp
// printing a comma separated list is annoying...
template<typename T>
ostream & operator<< (ostream & os, vector<T> const & v) {
    auto i = v.begin(), e = v.end();
    if (i != e) {
        os << *i++;
        for (; i != e; ++i) os << ", " << *i;
    }
    return os;
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    // for oranges demonstrate using a separate vector
    vector<string> oranges;
    cli.optVec(&oranges, "o orange").desc("oranges");
    // for apples demonstrate just using the proxy object
    auto & apples = cli.optVec<string>("[apple]").desc("red fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Comparing (" << *apples << ") and (" << *oranges << ").";
    return EX_OK;
}
~~~
View from the command line:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] [apple...]
  apple     red fruit

Options:
  -o, --orange=STRING  oranges

  --help               Show this message and exit.

$ a.out -o mandarin -onavel "red delicious" honeycrisp
Comparing (red delicious, honeycrisp) and (mandarin, navel).
~~~

While the * and -> operators get you full access to the underlying vector,
size() and [] are also available directly on the OptVec<T>. Which may
occasionally save a little bit of typing.

~~~ cpp
auto & apples = cli.optVec<string>("[apple]").desc("red fruit");
...
cout << "There were " << apples.size() << " apples." << endl;
if (apples)
    cout << "The first was " << apples[0] << endl;
~~~
