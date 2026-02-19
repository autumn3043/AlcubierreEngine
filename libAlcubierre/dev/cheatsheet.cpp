//Everything should be named according to camelCase. Capitalise the first letter of the names of classes, structs and enums.

class Example {
    public:
        Example(int _x) : x(_x) {} //Const members invalidate move constructors unless they are part of the initialisation list in the declaration.
        //The initialisation list must be accompanied by the function definition.
        //Initialisation list arguments are declared with _memberName, to indicate that they represent a temporary input value.
        //More loosely, values to be output in a similar vein may be declared with memberName_.
        ~Example(); //Classes which declare a destructor do not receive an automatic move constructor.

        //Copy constructors look like this:
        Example(const Example& other);
        //Copy assignment:
        Example& operator=(const Buffer& other);

        //Move constructor:
        Example(Example&& other);
        //Move assignment:
        Example& operator=(Example&& other); 

        //Operator inheritors are defined thusly:
        Example& operator+=(Example& other);
        bool operator==(Example& other) {
            return x == other.x;
        }
        //By convention (but not by spec) comparison operators return a bool, whereas arithmatic operators return Example&.

    private:
        const int x;
};
