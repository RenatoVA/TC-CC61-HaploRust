; ModuleID = 'HaploRustModule'
source_filename = "HaploRustModule"

@fmt = private unnamed_addr constant [5 x i8] c"%lf\0A\00", align 1

declare i32 @printf(ptr, ...)

declare double @exp(double)

define i32 @main() {
entry:
  %i = alloca i32, align 4
  store i32 0, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %i1 = load i32, ptr %i, align 4
  %cmp = icmp slt i32 %i1, 5
  br i1 %cmp, label %while.body, label %while.exit

while.body:                                       ; preds = %while.cond
  %i2 = load i32, ptr %i, align 4
  %int_to_double = sitofp i32 %i2 to double
  %printf_call = call i32 (ptr, ...) @printf(ptr @fmt, double %int_to_double)
  %i3 = load i32, ptr %i, align 4
  %addtmp = add i32 %i3, 1
  store i32 %addtmp, ptr %i, align 4
  br label %while.cond

while.exit:                                       ; preds = %while.cond
  ret i32 0
}
