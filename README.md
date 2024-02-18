# Project_IVM

Deze code implementeert optimalisatiemodellen voor de bepaling van de beste routes en ophaalkalenders voor afvalophaling.

Het eerste model is "allocatiepre". Dit model maakt een kalender die de ophaling zo gelijk mogelijk spreidt.
Het tweede model is "routing". Dit model bepaalt de optimale routes gegeven een ophaalkalender.
Het derde model is "allocatiepost". Dit model wijst gegenereerde ophaalroutes toe aan ophaaldagen om een ophaalkalender te maken.
De MIP-solver die wordt gebruik is CPLEX.