#!/bin/bash

echo "=== NERD Enhanced Features Demo ==="
echo "This demo shows the new workflow, persistence, and CLI features"
echo ""

# Test 1: Basic functionality
echo "1. Testing basic nerd functionality..."
echo "help" | timeout 5s ./build/nerd 2>/dev/null | head -10
echo ""

# Test 2: Test runner
echo "2. Running test suite..."
./build/nerd_test_runner --suite flow_tests 2>/dev/null | head -20
echo ""

# Test 3: Show system capabilities
echo "3. System capabilities:"
echo "- Enhanced CLI with color output and better error handling"
echo "- Workflow system for automated flow management"
echo "- Cost tracking and optimization"
echo "- Flow persistence for network disruption recovery"
echo "- Comprehensive testing framework"
echo ""

# Test 4: Show available commands
echo "4. Available commands:"
echo "  Flow Operations: open, close, list, status, info"
echo "  Workflow Operations: workflow list, workflow run, cost"
echo "  Persistence: save, load, backup, restore, checkpoint"
echo "  System: system status, system info, config show"
echo "  Utilities: history, clear, export, import"
echo ""

echo "=== Demo Complete ==="
echo "To run the full interactive demo:"
echo "  sudo ./build/nerd"
echo ""
echo "To run tests:"
echo "  ./build/nerd_test_runner --verbose"
echo ""
echo "For more information, see ENHANCED_FEATURES.md"