<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Positional Arguments
A few things to keep in mind about positional arguments:

- Positional arguments are mapped by the order they are added, except that
  required ones appear before optional ones.
- If there are multiple vector positionals with unlimited (nargs = -1) arity
  all but the first will be treated as if they had nargs = 1.
- If the unlimited one is required it will prevent any optional positionals
  from getting populated, since it eats up all the arguments before they get
  a turn.
