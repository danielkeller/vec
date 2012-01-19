namespace Rejigger {
	bool DoFunction(Func * f);
	
	void Clear(); //reset internal state info

	TypeWrapper * resolve(string name);
	
	extern Func * func_cur;
	extern vector<Block*> blocks_cur;
	extern vector<Stmt*> init_stmts;
	extern vector<TmpUser*> tmps;
	
	extern vector<VarDecl*>::iterator curdecl;
//	extern vector<Stmt*>::iterator curstmt;
};
