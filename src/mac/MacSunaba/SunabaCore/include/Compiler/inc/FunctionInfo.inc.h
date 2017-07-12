namespace Sunaba{

//FunctionInfo
inline FunctionInfo::FunctionInfo() : 
mArgCount(0), 
mHasOutputValue(false){
}

inline void FunctionInfo::setHasOutputValue(){
	mHasOutputValue = true;
}

inline bool FunctionInfo::hasOutputValue() const{
	return mHasOutputValue;
}

inline int FunctionInfo::argCount() const{
	return mArgCount;
}

inline void FunctionInfo::setArgCount(int a){
	mArgCount = a;
}

} //namespace Sunaba
