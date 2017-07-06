#!/bin/sh
cd ../..
rm -f SunabaSystemError.txt
rm -rf $(find ./ -name "log" -or -name "obj" -or -name "ipch" -and -type d)
rm -rf $(find ./ -name "*.ilk" -or -name "*~" -or -name "*.user" -or -name "*.pdb" -or -name "*.txt~" -or -name "*.msi" -or -name "*.sdf" -or -name "*.opensdf" -or -name "*.aux" -or -name "*.out" -or -name "*.idx" -or -name "*.toc" -or -name "*.dll.metagen" -or -name "*.xbb" -or -name "*.dvi" -or -name "*.ilg" -or -name "*.ind" -or -name "*.log")
