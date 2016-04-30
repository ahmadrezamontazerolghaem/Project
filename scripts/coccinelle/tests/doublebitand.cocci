@r expression@
expression E;
position p;
@@

(
*        E@p
         & ... & E
|
*        E@p 
         | ... | E
|
*        E@p
         & ... & !E
|
*        E@p
         | ... | !E
|
*        !E@p
         & ... & E
|
*        !E@p
         | ... | E
)

@script:python depends on org@
p << r.p;
@@

cocci.print_main("duplicated argument to & or |",p)

@script:python depends on report@
p << r.p;
@@

coccilib.report.print_report(p[0],"duplicated argument to & or |")
