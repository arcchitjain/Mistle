0.5::a.
0.2::b; 0.2::c; 0.2::d; 0.2::e; 0.2::f.
0.5::g; 0.5::h.
0.5::i.
0.5::j.
0.5::k.
0.5::l; 0.5::m.
0.5::n.

0.5::z1.
0.5::z2.
0.5::z3.
0.5::z4.
0.5::z5.

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

evidence(a).
evidence(b).
evidence(g).
evidence(i).
evidence(j).

query(y).

