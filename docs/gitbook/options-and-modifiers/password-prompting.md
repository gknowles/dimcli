<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Password Prompting
The fPromptHide and fPromptConfirm options are especially handy when asking
for passwords.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & pass = cli.opt<string>("password")
        .prompt(cli.fPromptHide | cli.fPromptConfirm);
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Password was: " << *pass;
    return EX_OK;
}
~~~
Results in:

~~~ console
$ a.out
Password:
Enter again to confirm:
Password was: secret
~~~
For passwords you can use opt.passwordOpt() instead of spelling it out.

~~~ cpp
auto & pass = cli.passwordOpt(/*confirm=*/true);
~~~
Which gives you:

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --password=STRING  Password required for access.

  --help             Show this message and exit.
~~~
