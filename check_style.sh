#!/bin/bash
# Manual style checker for common Zephyr coding style issues

echo "Checking coding style for IS31FL3235A driver..."
echo "================================================"
echo ""

errors=0
warnings=0

# Check for trailing whitespace
echo "Checking for trailing whitespace..."
if grep -n ' $' driver/is31fl3235a.c driver/is31fl3235a_regs.h include/is31fl3235a.h 2>/dev/null; then
    echo "  ❌ Found trailing whitespace"
    ((errors++))
else
    echo "  ✓ No trailing whitespace"
fi
echo ""

# Check for lines longer than 100 characters
echo "Checking line length (max 100 characters)..."
long_lines=$(awk 'length > 100 {print NR": "substr($0,1,80)"..."}' driver/is31fl3235a.c)
if [ -n "$long_lines" ]; then
    echo "  ⚠️  Long lines found in is31fl3235a.c:"
    echo "$long_lines" | head -5
    ((warnings++))
else
    echo "  ✓ All lines ≤ 100 characters in is31fl3235a.c"
fi
echo ""

# Check for proper SPDX header
echo "Checking SPDX license headers..."
for file in driver/is31fl3235a.c driver/is31fl3235a_regs.h include/is31fl3235a.h; do
    if head -5 "$file" | grep -q "SPDX-License-Identifier: Apache-2.0"; then
        echo "  ✓ $file has SPDX header"
    else
        echo "  ❌ $file missing SPDX header"
        ((errors++))
    fi
done
echo ""

# Check for spaces instead of tabs (in leading whitespace)
echo "Checking for spaces vs tabs in indentation..."
if grep -n '^        ' driver/is31fl3235a.c | head -1; then
    echo "  ⚠️  Possible spaces used for indentation (should use tabs)"
    ((warnings++))
else
    echo "  ✓ Using tabs for indentation"
fi
echo ""

# Check for proper include guard format in headers
echo "Checking include guards..."
for file in driver/is31fl3235a_regs.h include/is31fl3235a.h; do
    if head -10 "$file" | grep -q "#ifndef.*_H_$"; then
        echo "  ✓ $file has include guard"
    else
        echo "  ⚠️  $file may have non-standard include guard"
        ((warnings++))
    fi
done
echo ""

# Check for proper Doxygen comments
echo "Checking for Doxygen comments..."
if grep -q "@brief" driver/is31fl3235a.c; then
    echo "  ✓ Found Doxygen comments"
else
    echo "  ⚠️  No Doxygen comments found"
    ((warnings++))
fi
echo ""

# Check for LOG_MODULE_REGISTER
echo "Checking for logging setup..."
if grep -q "LOG_MODULE_REGISTER" driver/is31fl3235a.c; then
    echo "  ✓ Logging module registered"
else
    echo "  ❌ No LOG_MODULE_REGISTER found"
    ((errors++))
fi
echo ""

# Summary
echo "================================================"
echo "Style Check Summary:"
echo "  Errors:   $errors"
echo "  Warnings: $warnings"
echo ""

if [ $errors -eq 0 ] && [ $warnings -eq 0 ]; then
    echo "✅ All checks passed!"
    exit 0
elif [ $errors -eq 0 ]; then
    echo "⚠️  Passed with warnings"
    exit 0
else
    echo "❌ Failed with errors"
    exit 1
fi
