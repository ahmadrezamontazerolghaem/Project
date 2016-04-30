@depends on patch@
iterator I;
expression x,E,E1,E2;
statement S,S1,S2;
@@

I(x,...) { <...
(
- if (x == NULL && ...) S
| 
- if (x != NULL || ...)
  S
|
- (x == NULL) ||
  E
|
- (x != NULL) &&
  E
|
- (x == NULL && ...) ? E1 :
  E2
|
- (x != NULL || ...) ?
  E1
- : E2
|
- if (x == NULL && ...) S1 else
  S2
|
- if (x != NULL || ...)
  S1
- else S2
|
+ BAD(
  x == NULL
+ )
|
+ BAD(
  x != NULL
+ )
)
  ...> }

@r depends on !patch exists@
iterator I;
expression x,E;
position p1,p2;
@@

*I@p1(x,...)
{ ... when != x = E
(
*  x@p2 == NULL
|
*  x@p2 != NULL
)
  ... when any
}

@script:python depends on org@
p1 << r.p1;
p2 << r.p2;
@@

cocci.print_main("iterator-bound variable",p1)
cocci.print_secs("useless NULL test",p2)

@script:python depends on report@
p1 << r.p1;
p2 << r.p2;
@@

msg = "ERROR: iterator variable bound on line %s cannot be NULL" % (p1[0].line)
coccilib.report.print_report(p2[0], msg)
