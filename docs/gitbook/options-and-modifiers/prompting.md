<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Prompting
You can have an option prompt the user for the value when it's left off of
the command line.

In addition to simple prompting, there are some flags that modify the behavior.

| Flag             | Description                        |
|------------------|------------------------------------|
| fPromptHide      | Hide the input from the console    |
| fPromptConfirm   | Require the value be entered twice |
| fPromptNoDefault | Don't show the default             |

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & cookies = cli.opt<int>("cookies c").prompt();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "There are " << *cookies << " cookies.";
    return EX_OK;
}
~~~
By default the prompt is a capitalized version of the first option name.
Which is why this example uses "cookies c" instead of "c cookies".

~~~ console
$ a.out -c5
There are 5 cookies.
$ a.out
Cookies [0]: 3
There are 3 cookies.
~~~
The first option name is also used in errors where no name is available from
the command line, such as when the value is from a prompt. The following
fails because "nine" isn't an int.

~~~ console
$ a.out
Cookies: nine
Error: Invalid '--cookies' value: nine
~~~
You can change the prompt to something more appropriate and hide the default:

~~~ cpp
auto & cookies = cli.opt<int>("cookies c")
    .prompt("How many cookies did you buy?", cli.fPromptNoDefault);
~~~
Which gives you:

~~~ console
$ a.out
How many cookies did you buy? 9
There are 9 cookies.
~~~
