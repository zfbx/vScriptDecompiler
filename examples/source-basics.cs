// Random Comment
exec("./echo_hi.cs");
exec("./arrays.cs");
echo("Loading basics.cs");
$GlobalInt = 42;
$GlobalString = "Don't Panic";
$Characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!@#$%^&*()_-+=[]{}';:,./\\|\"";

$myArray[0] = "Hello";
$myArray[1] = "TorqueScript";

package basics {

    function testMath(%a, %b) {
        echo("Test Math with " @ %a @ " and " @ %b);
        echo(%a + %b);
        echo(%a - %b);
        echo(%a * %b);
        echo(%a / %b);
        echo(%a % %b);
        echo(%a | %b);
        echo(%a >> %b);
        echo(%a << %b);
    }

    function testLogic(%a, %b) {
        echo("Test Logic with " @ %a @ " and " @ %b);
        echo(%a > %b);
        echo(!(%a > %b));
        echo(%a < %b);
        echo(%a == %b);
        echo(%a >= %b);
        echo(%a <= %b);
        echo(%a || %b);
        echo(%a && %b);
        echo(%a ^ %b);

        if (%a > %b) {
            echo("A is greater.");
        } else if (%a < %b) {
            echo("B is greater.");
        } else {
            echo("A and B are the same.");
        }

        // Void Return
        if (true) {
            return;
        }

        echo("Early Return failed");
    }

    function testStrings(%a, %b) {
        echo("Test Strings with " @ %a @ " and " @ %b);
        echo(%a $= %b);
    }

    function testObject() {
        %obj = new SimObject(MySimObj) {
            myProperty = "TestValue";
        };
        echo("Created object: " @ %obj.getName() @ " with property: " @ %obj.myProperty);
        return %obj;
    }

    function testForLoop() {
        for (%i = 0; %i < 2; %i++) {
            echo("Array[" @ %i @ "] = " @ $myArray[%i]);
        }
    }

};

activatePackage(basics);

echo("Hi" NL "There" TAB "Person" SPC "You");

basics::testMath(10, $GlobalInt);
basics::testLogic(10, $GlobalInt);
basics::testLogic($GlobalInt, 10);
basics::testStrings($GlobalString, "Whale");
basics::testObject();
basics::testForLoop();
