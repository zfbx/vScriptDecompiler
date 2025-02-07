//Simulated 2x2 Array
$myArray["0,0"] = "Row0-Col0";
$myArray["0,1"] = "Row0-Col1";
$myArray["1,0"] = "Row1-Col0";
$myArray["1,1"] = "Row1-Col1";

echo($myArray["0,1"]);

// SimSet array
%outerSet = new SimSet();

%row0 = new SimSet();
%row1 = new SimSet();

%row0.add("Row0-Col0");
%row0.add("Row0-Col1");

%row1.add("Row1-Col0");
%row1.add("Row1-Col1");

%outerSet.add(%row0);
%outerSet.add(%row1);

%firstRow = %outerSet.getObject(0);
%element = %firstRow.getObject(1);
echo(%element);