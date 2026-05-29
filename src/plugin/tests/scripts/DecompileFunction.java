// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public class DecompileFunction extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String functionName = args.length > 0 ? args[0] : "main";
        Path outputPath = Path.of(args.length > 1 ? args[1] : "decompiled.c");

        List<Function> functions = getGlobalFunctions(functionName);
        if (functions.isEmpty()) {
            throw new IllegalStateException("function not found: " + functionName);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        DecompileResults results =
            decompiler.decompileFunction(functions.get(0), 120, monitor);
        if (!results.decompileCompleted()) {
            throw new IllegalStateException(
                "decompile failed: " + results.getErrorMessage()
            );
        }

        Files.writeString(outputPath, results.getDecompiledFunction().getC());
        decompiler.dispose();
    }
}
