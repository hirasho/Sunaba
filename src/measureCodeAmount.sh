wc $(find ./ -name "*.cpp" -or -name "*.h" -or -name "*.cs") |sort -nr |unix2dos > codeAmount.txt
