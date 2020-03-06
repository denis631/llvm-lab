import lldb

def ValueFormatter(valobj, internal_dict):
    frame = lldb.debugger.GetSelectedTarget().GetProcess().GetSelectedThread().GetSelectedFrame()
    options = lldb.SBExpressionOptions()
    options.SetLanguage(lldb.eLanguageTypeObjC_plus_plus)
    options.SetTrapExceptions(False)
    options.SetTimeoutInMicroSeconds(5000000) # 5s

    addr = valobj.GetValue()
    expr = frame.EvaluateExpression('((llvm::Value*) {0})->getName().data()'.format(addr), options)
    name = expr.GetSummary()
    return '{0}'.format(name)

def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand("type summary add -F " + __name__ + ".ValueFormatter  llvm::Value")
