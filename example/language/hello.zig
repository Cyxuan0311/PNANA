// Zig Hello World Example
// Demonstrates Zig syntax features for syntax highlighting

const std = @import("std");

/// Application name constant
const APP_NAME = "Zig Example";
const VERSION = "1.0.0";

/// Person struct with optional email
const Person = struct {
    name: []const u8,
    age: u32,
    email: ?[]const u8,

    pub fn greet(self: *const Person) void {
        std.debug.print("Hello, {s}! You are {d} years old.\n", .{ self.name, self.age });
        if (self.email) |e| {
            std.debug.print("  Email: {s}\n", .{e});
        }
    }
};

/// Generic pair structure
fn Pair(comptime T: type, comptime U: type) type {
    return struct {
        first: T,
        second: U,
    };
}

pub fn main() !void {
    std.debug.print("=== {s} v{s} ===\n\n", .{ APP_NAME, VERSION });

    // Defer runs when scope exits (LIFO order)
    defer std.debug.print("Example completed.\n", .{});

    var person = Person{
        .name = "World",
        .age = 2024,
        .email = "hello@zig.example",
    };
    person.greet();

    // Simple loop
    var i: u32 = 0;
    while (i < 5) : (i += 1) {
        std.debug.print("Count: {d}\n", .{i});
    }

    // For loop over slice
    const nums = [_]u32{ 10, 20, 30 };
    for (nums) |n| {
        std.debug.print("Number: {d}\n", .{n});
    }

    // Conditional
    const number: i32 = 42;
    if (number > 40) {
        std.debug.print("Number is greater than 40\n", .{});
    } else if (number > 30) {
        std.debug.print("Number is greater than 30\n", .{});
    } else {
        std.debug.print("Number is small\n", .{});
    }

    // Error union and try
    const result = computeSomething(10) catch |err| {
        std.debug.print("Error: {}\n", .{err});
        return err;
    };
    std.debug.print("Result: {d}\n", .{result});

    // Optional and orelse
    var maybe_val: ?u32 = 99;
    const val = maybe_val orelse 0;
    std.debug.print("Value: {d}\n", .{val});

    // Comptime
    const pair = Pair(i32, []const u8){
        .first = 42,
        .second = "zig",
    };
    std.debug.print("Pair: {d}, {s}\n", .{ pair.first, pair.second });
}

fn computeSomething(n: u32) !u32 {
    if (n == 0) return error.InvalidInput;
    return n * 2;
}

// Test block
test "basic math" {
    try std.testing.expect(2 + 2 == 4);
}

test "person greet" {
    const p = Person{
        .name = "Test",
        .age = 1,
        .email = null,
    };
    try std.testing.expect(p.age == 1);
}
