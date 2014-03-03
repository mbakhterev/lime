text=$(cat)
cat <<EOF
(
	.TEnv ('0.7."pointer"; .T ('0.1."P"; 4));
	.TEnv ('0.5."float"; .T ('0.1."F"; 4));
	.TEnv ('0.6."double"; .T ('0.1."F"; 4));
	.TEnv ('0.3."i64"; .T ('0.1."I"; 8));
	.TEnv ('0.3."i32"; .T ('0.1."I"; 4));
	.TEnv ('0.3."i16"; .T ('0.1."I"; 2));
	.TEnv ('0.2."i8"; .T ('0.1."I"; 1));
	.TEnv ('0.2."i1"; .T ('0.1."I"; 1));

// PROTOTYPES
	.E ('0.3."ENV"; ('0.4."this"; '0.2."fn"; '0.6."parent"); .E ('0.3."ENV";
('0.4."this")));
	.S ('0.2."fn";.T ('0.8."function";.T ('0.4."void"); (.T ('0.4."void"))));
	.E ('0.3."ENV";
		 ('0.4."this"; '0.2."fn"; '0.4."this");
		 .E ('0.3."ENV"; ('0.4."this"; '0.2."fn")));
	.proc (
		.S ('0.2."fn");
		.Env ( .E ('0.3."ENV"; ('0.4."this"; '0.2."fn"));
		.LB (
			.LB spec (
				.S sprv ('0.5."SP.RV"; .T ('0.3."RET"))
			);
			// Function body
			.LB l0 
${text};
		.Label ('0.2."l0"; l0)
		) // End ENV LB
		) // End ENV
	)// end proc
)
EOF
