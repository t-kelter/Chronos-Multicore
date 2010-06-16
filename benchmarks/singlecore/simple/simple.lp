Maximize

66 dSta_0 + 8 d0_1.0 + 66 d0_1.1 + 0 d0_1 + 0 d0_1.0 + 0 d0_1.0 + 1 dc1_2.0 + 30 dc1_2.1 + 0 dc1_2 + 52 dc1_2.0 + 0 dc1_2.0 + 4 dm1_2.0 + 34 dm1_2.1 + 0 dm1_2 + 52 dm1_2.0 + 0 dm1_2.0 + 8 dc1_1.0 + 66 dc1_1.1 + 0 dc1_1 + 52 dc1_1.0 + 0 dc1_1.0 + 11 dm1_1.0 + 69 dm1_1.1 + 0 dm1_1 + 52 dm1_1.0 + 0 dm1_1.0


Subject to

bm - dm1_1 - dm1_2 = 0
cm - 2 d0_1.1 - 2 d1_1.1 - d1_2.1 = 2

\ === tcfg constraints ===
dSta_0 = 1
b0 - d0_1 = 0
b0 - dSta_0 = 0
b1 - d1_2 - d1_1 = 0
b1 - d0_1 - d1_1 = 0
b2 - d1_2 = 0

c0 - f0_1T <= 1
c0 - f0_1T >= 0
c0 <= 1
c0 >= 0
c1 - f1_2T <= 1
c1 - f1_2T >= 0
c1 - f0_1T <= 0
c1 - f0_1T >= 0
c2 - f2_2T <= 1
c2 - f2_2T >= 0
c2 - f2_2T - f1_2T <= 0
c2 - f2_2T - f1_2T >= 0


f0_1T - fc0_1T - fm0_1T = 0
f1_2T - fc1_2T - fm1_2T = 0
f2_2T - fc2_2T - fm2_2T = 0

c0 <= 1
c0 >= 0
c1 <= 1
c1 >= 0
c2 - p2_2T <= 1
c2 - p2_2T >= 0

f0_1T <= 1
f0_1T >= 0
f1_2T <= 1
f1_2T >= 0
f2_2T - p2_2T <= 0
f2_2T - p2_2T >= 0

fm0_1T <= 1
fm1_2T <= 1
fm2_2T <= 1

b1 - c0 - c2 - c1 = 0
d1_2 - dc1_2 - dm1_2 = 0
d1_1 - dc1_1 - dm1_1 = 0
d1_2 <= 1
dm1_2 <= 1
d1_1 - f0_1T - f2_2T - f1_2T = 0
dm1_1 - fm0_1T - fm2_2T - fm1_2T = 0


\ === cache_cons ===
b0.0 <= 1
b1 - b1.0 - b1.1 = 0
b2 - b2.0 - b2.1 = 0

b0.0 <= 1
b1.0 - d0_1.0 - d1_1.0 = 0
b1.1 - d0_1.1 - d1_1.1 = 0
b2.0 - d1_2.0 = 0
b2.1 - d1_2.1 = 0

b0.1 <= 1
b1.1 <= 1
b2.1 <= 1

d1_1.1 = 0

\ === mpset_cons ===

\ === mpunit_cons ===

\ === tcfg_estunit_cons ===
d0_1 - d0_1.0 - d0_1.1 = 0
d1_2 - d1_2.0 - d1_2.1 = 0
d1_2.0 - dc1_2.0 - dm1_2.0 = 0
d1_2.1 - dc1_2.1 - dm1_2.1 = 0
dc1_2 - dc1_2.0 - dc1_2.1 = 0
dm1_2 - dm1_2.0 - dm1_2.1 = 0
d1_1 - d1_1.0 - d1_1.1 = 0
d1_1.0 - dc1_1.0 - dm1_1.0 = 0
d1_1.1 - dc1_1.1 - dm1_1.1 = 0
dc1_1 - dc1_1.0 - dc1_1.1 = 0
dm1_1 - dm1_1.0 - dm1_1.1 = 0

\ === tcfg_estunit_cons_ps ===
d0_1 - d0_1.0 >= 0
d0_1.0 <= 1
d1_2 - d1_2.0 >= 0
d1_2.0 <= 1
d1_1 - d1_1.0 >= 0
d1_1.0 <= 1

\ === infeasible path constraints ===

b0 = 1
b1 = 100
b2 = 1

Generals

bm
cm
Sta
b0
d0_1
b1
d1_2
d1_1
b2
c0
f0_1T
f0_1T
c1
f1_2T
f1_2T
c2
f2_2T
f2_2T
fc0_1T
fm0_1T
fc1_2T
fm1_2T
fc2_2T
fm2_2T
dc1_2
dm1_2
dc1_1
dm1_1
b0.0
b1.0
b1.1
b2.0
b2.1
b0.0
b1.0
b1.1
b2.0
b2.1

End
\ total_cons: 76 	total_vars: 38
