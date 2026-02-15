// C# 示例文件
// 展示 C# 语言的各种语法特性

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.IO;
using System.Text.Json;
using System.Text.RegularExpressions;

// 命名空间
namespace HelloCSharp
{
    // 全局常量
    public static class Constants
    {
        public const string AppName = "C# Example";
        public const string Version = "1.0.0";
        public const double Pi = 3.14159;
    }

    // 枚举
    public enum Status
    {
        Pending,
        Approved,
        Rejected
    }

    // 接口
    public interface IShape
    {
        double Area { get; }
        double Perimeter { get; }
        void Draw();
    }

    // 抽象类
    public abstract class Shape : IShape
    {
        public abstract double Area { get; }
        public abstract double Perimeter { get; }

        public virtual void Draw()
        {
            Console.WriteLine($"Drawing a {GetType().Name}");
        }

        public abstract string GetDescription();
    }

    // 结构体
    public struct Point
    {
        public int X { get; set; }
        public int Y { get; set; }

        public Point(int x, int y)
        {
            X = x;
            Y = y;
        }

        public double DistanceTo(Point other)
        {
            int dx = X - other.X;
            int dy = Y - other.Y;
            return Math.Sqrt(dx * dx + dy * dy);
        }

        public override string ToString() => $"({X}, {Y})";
    }

    // 类
    public class Circle : Shape
    {
        public double Radius { get; private set; }

        public override double Area => Math.PI * Radius * Radius;
        public override double Perimeter => 2 * Math.PI * Radius;

        public Circle(double radius)
        {
            if (radius <= 0)
                throw new ArgumentException("Radius must be positive", nameof(radius));
            Radius = radius;
        }

        public override string GetDescription() => $"Circle with radius {Radius}";
    }

    // 记录类型 (C# 9.0+)
    public record Person(string Name, int Age, string? Email = null)
    {
        public bool IsAdult => Age >= 18;

        public Person CelebrateBirthday() => this with { Age = Age + 1 };

        public string Greet() => $"Hello, I'm {Name} and I'm {Age} years old!";
    }

    // 泛型类
    public class Repository<T> where T : class
    {
        private readonly List<T> _items = new();

        public void Add(T item)
        {
            if (item == null)
                throw new ArgumentNullException(nameof(item));
            _items.Add(item);
        }

        public T? Find(Predicate<T> predicate)
        {
            return _items.FirstOrDefault(item => predicate(item));
        }

        public IEnumerable<T> GetAll() => _items.AsEnumerable();

        public int Count => _items.Count;
    }

    // 委托
    public delegate void EventHandler<T>(object sender, T e);

    // 事件
    public class EventPublisher
    {
        public event EventHandler<string>? MessageReceived;

        protected virtual void OnMessageReceived(string message)
        {
            MessageReceived?.Invoke(this, message);
        }

        public void Publish(string message)
        {
            Console.WriteLine($"Publishing: {message}");
            OnMessageReceived(message);
        }
    }

    // 主程序类
    public class Program
    {
        // 静态变量
        private static int _globalCounter = 0;

        // 主方法
        public static async Task Main(string[] args)
        {
            Console.WriteLine("=== C# Example ===\n");

            DemonstrateBasicTypes();
            DemonstrateCollections();
            DemonstrateOOP();
            DemonstrateGenerics();
            DemonstrateLINQ();
            DemonstrateAsyncAwait();
            DemonstrateExceptionHandling();
            DemonstrateFileOperations();
            DemonstratePatterns();
            DemonstrateRecords();
            DemonstrateDelegatesAndEvents();

            Console.WriteLine("\n=== C# Example Completed ===");
        }

        // 基本数据类型
        private static void DemonstrateBasicTypes()
        {
            Console.WriteLine("--- Basic Data Types ---");

            string str = "Hello, C# World!";
            int num = 42;
            double floatNum = 3.14159;
            bool boolValue = true;
            char ch = 'C';
            decimal money = 99.99m;
            DateTime now = DateTime.Now;
            TimeSpan duration = TimeSpan.FromHours(2);

            Console.WriteLine($"String: {str}");
            Console.WriteLine($"Integer: {num}");
            Console.WriteLine($"Double: {floatNum}");
            Console.WriteLine($"Boolean: {boolValue}");
            Console.WriteLine($"Char: {ch}");
            Console.WriteLine($"Decimal: {money:C}");
            Console.WriteLine($"DateTime: {now:yyyy-MM-dd HH:mm:ss}");
            Console.WriteLine($"TimeSpan: {duration}");
            Console.WriteLine();
        }

        // 集合
        private static void DemonstrateCollections()
        {
            Console.WriteLine("--- Collections ---");

            // 数组
            int[] numbers = { 1, 2, 3, 4, 5 };
            Console.WriteLine($"Array: [{string.Join(", ", numbers)}]");

            // List
            var list = new List<int> { 10, 20, 30, 40, 50 };
            list.Add(60);
            Console.WriteLine($"List: [{string.Join(", ", list)}]");

            // Dictionary
            var dict = new Dictionary<string, int>
            {
                ["one"] = 1,
                ["two"] = 2,
                ["three"] = 3
            };
            Console.WriteLine($"Dictionary: {string.Join(", ", dict.Select(kvp => $"{kvp.Key}={kvp.Value}"))}");

            // HashSet
            var set = new HashSet<int> { 1, 2, 3, 3, 4, 4, 5 };
            Console.WriteLine($"HashSet: [{string.Join(", ", set)}]");

            // Queue
            var queue = new Queue<string>();
            queue.Enqueue("first");
            queue.Enqueue("second");
            queue.Enqueue("third");
            Console.WriteLine($"Queue: {queue.Dequeue()}");

            // Stack
            var stack = new Stack<int>();
            stack.Push(1);
            stack.Push(2);
            stack.Push(3);
            Console.WriteLine($"Stack: {stack.Pop()}");
            Console.WriteLine();
        }

        // 面向对象编程
        private static void DemonstrateOOP()
        {
            Console.WriteLine("--- Object-Oriented Programming ---");

            var circle = new Circle(5.0);
            Console.WriteLine($"Circle Area: {circle.Area:F2}");
            Console.WriteLine($"Circle Perimeter: {circle.Perimeter:F2}");
            Console.WriteLine(circle.GetDescription());
            circle.Draw();

            var point1 = new Point(0, 0);
            var point2 = new Point(3, 4);
            Console.WriteLine($"Distance between {point1} and {point2}: {point1.DistanceTo(point2):F2}");
            Console.WriteLine();
        }

        // 泛型
        private static void DemonstrateGenerics()
        {
            Console.WriteLine("--- Generics ---");

            var repo = new Repository<Person>();
            repo.Add(new Person("Alice", 25, "alice@example.com"));
            repo.Add(new Person("Bob", 30, "bob@example.com"));
            repo.Add(new Person("Charlie", 17));

            var adult = repo.Find(p => p.IsAdult);
            Console.WriteLine($"Found adult: {adult?.Name}");
            Console.WriteLine($"Total persons: {repo.Count}");
            Console.WriteLine();
        }

        // LINQ
        private static void DemonstrateLINQ()
        {
            Console.WriteLine("--- LINQ ---");

            var numbers = new[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

            // Where
            var evens = numbers.Where(n => n % 2 == 0);
            Console.WriteLine($"Even numbers: [{string.Join(", ", evens)}]");

            // Select
            var squares = numbers.Select(n => n * n);
            Console.WriteLine($"Squares: [{string.Join(", ", squares)}]");

            // Aggregate
            var sum = numbers.Aggregate(0, (acc, n) => acc + n);
            Console.WriteLine($"Sum: {sum}");

            // GroupBy
            var grouped = numbers.GroupBy(n => n % 2 == 0 ? "Even" : "Odd");
            foreach (var group in grouped)
            {
                Console.WriteLine($"{group.Key}: [{string.Join(", ", group)}]");
            }

            // OrderBy
            var sorted = numbers.OrderByDescending(n => n);
            Console.WriteLine($"Sorted descending: [{string.Join(", ", sorted)}]");
            Console.WriteLine();
        }

        // 异步编程
        private static async Task DemonstrateAsyncAwait()
        {
            Console.WriteLine("--- Async/Await ---");

            async Task<string> FetchDataAsync()
            {
                await Task.Delay(100);
                return "Data fetched successfully!";
            }

            var result = await FetchDataAsync();
            Console.WriteLine(result);

            // Parallel processing
            var tasks = new[]
            {
                Task.Run(async () => { await Task.Delay(100); return "Task 1"; }),
                Task.Run(async () => { await Task.Delay(150); return "Task 2"; }),
                Task.Run(async () => { await Task.Delay(200); return "Task 3"; })
            };

            var results = await Task.WhenAll(tasks);
            Console.WriteLine($"All tasks completed: {string.Join(", ", results)}");
            Console.WriteLine();
        }

        // 异常处理
        private static void DemonstrateExceptionHandling()
        {
            Console.WriteLine("--- Exception Handling ---");

            try
            {
                int divisor = 0;
                int result = 10 / divisor;
            }
            catch (DivideByZeroException ex)
            {
                Console.WriteLine($"Caught exception: {ex.Message}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"General exception: {ex.Message}");
            }
            finally
            {
                Console.WriteLine("Finally block executed");
            }

            // Using statement for resource management
            using (var stream = new MemoryStream())
            {
                var data = System.Text.Encoding.UTF8.GetBytes("Hello, C#!");
                stream.Write(data, 0, data.Length);
                Console.WriteLine($"Stream length: {stream.Length}");
            } // Stream automatically disposed

            Console.WriteLine();
        }

        // 文件操作
        private static void DemonstrateFileOperations()
        {
            Console.WriteLine("--- File Operations ---");

            string fileName = "example.txt";
            string content = "Hello, C# File I/O!";

            // Write file
            File.WriteAllText(fileName, content);
            Console.WriteLine($"Written to {fileName}");

            // Read file
            string readContent = File.ReadAllText(fileName);
            Console.WriteLine($"Read from {fileName}: {readContent}");

            // Check if file exists
            if (File.Exists(fileName))
            {
                File.Delete(fileName);
                Console.WriteLine($"Deleted {fileName}");
            }

            Console.WriteLine();
        }

        // 模式匹配 (C# 7.0+)
        private static void DemonstratePatterns()
        {
            Console.WriteLine("--- Pattern Matching ---");

            object obj = 42;

            // Type pattern
            if (obj is int number)
            {
                Console.WriteLine($"It's an integer: {number}");
            }

            // Switch expression
            string result = obj switch
            {
                int i => $"Integer: {i}",
                string s => $"String: {s}",
                double d => $"Double: {d}",
                _ => "Unknown type"
            };
            Console.WriteLine(result);

            // Property pattern
            var person = new Person("Alice", 25);
            string message = person switch
            {
                { Age: < 18 } => "Minor",
                { Age: >= 18 and < 65 } => "Adult",
                { Age: >= 65 } => "Senior",
                _ => "Unknown"
            };
            Console.WriteLine($"Person category: {message}");
            Console.WriteLine();
        }

        // 记录类型
        private static void DemonstrateRecords()
        {
            Console.WriteLine("--- Records ---");

            var person1 = new Person("Alice", 25, "alice@example.com");
            var person2 = new Person("Alice", 25, "alice@example.com");
            var person3 = person1 with { Age = 26 };

            Console.WriteLine($"Person1: {person1}");
            Console.WriteLine($"Person2: {person2}");
            Console.WriteLine($"Person1 == Person2: {person1 == person2}"); // True (value equality)
            Console.WriteLine($"Person1 == Person3: {person1 == person3}"); // False
            Console.WriteLine($"Person1.IsAdult: {person1.IsAdult}");
            Console.WriteLine($"Person1.Greet(): {person1.Greet()}");
            Console.WriteLine();
        }

        // 委托和事件
        private static void DemonstrateDelegatesAndEvents()
        {
            Console.WriteLine("--- Delegates and Events ---");

            // Action delegate
            Action<string> printAction = (message) => Console.WriteLine($"Action: {message}");
            printAction("Hello from Action!");

            // Func delegate
            Func<int, int, int> addFunc = (a, b) => a + b;
            Console.WriteLine($"Func result: {addFunc(5, 3)}");

            // Predicate delegate
            Predicate<int> isEven = (n) => n % 2 == 0;
            Console.WriteLine($"Is 4 even? {isEven(4)}");

            // Event
            var publisher = new EventPublisher();
            publisher.MessageReceived += (sender, message) =>
            {
                Console.WriteLine($"Event received: {message}");
            };
            publisher.Publish("Hello from event!");
            Console.WriteLine();
        }
    }
}

