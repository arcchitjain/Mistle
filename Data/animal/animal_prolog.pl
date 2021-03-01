x :- \+l, m, z1.
x :- l, \+m, z1.
x :- i, k, z2.
x :- \+i, \+k, \+z1, z2.
x :- b, \+e, k, z3.
x :- \+b, e, \+k, \+z1, z3.
x :- g, \+h, z4.
x :- \+z4, \+b, \+c, d, \+e, \+f, i, \+j.
x :- \+z1, \+g, h, z4.
x :- \+z2, \+b, c, \+e, z5.
x :- \+z5, \+d, \+f, g, \+h, \+j.
x :- \+z3, \+c, \+i, z5.

y :- \+x.

a.
b.
g.
i.
j.