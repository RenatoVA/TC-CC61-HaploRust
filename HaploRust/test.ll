; ModuleID = 'HaploRustModule'
source_filename = "HaploRustModule"

@fmt = private unnamed_addr constant [5 x i8] c"%lf\0A\00", align 1

declare i32 @printf(ptr, ...)

declare double @exp(double)

define i32 @main() {
entry:
  %printf_call = call i32 (ptr, ...) @printf(ptr @fmt, double 7.000000e+00)
  ret i32 0
}
