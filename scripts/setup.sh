if ! command -v pre-commit &>/dev/null; then
    echo "[!] Installing pre-commit..."
    if command -v pip &>/dev/null; then
        pip install pre-commit
    elif command -v pip3 &>/dev/null; then
        pip3 install pre-commit
    else
        echo "ERROR: pip not found. Please install Python and pip first."
        exit 1
    fi
else
    echo "[!] pre-commit already installed ($(pre-commit --version))"
fi

echo "[!] Installing pre-commit git hook..."
pre-commit install
pre-commit install --hook-type commit-msg

echo ""
echo "[!] Configuring CMake..."
cmake -S . -B build

echo ""
echo "[!] Done! To build the project run:"
echo "\tcmake --build build"