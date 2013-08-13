#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

#include <XElement.h>
#include <XDocument.h>

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace SCX::Util;
using namespace SCX::Util::Xml;
using std::string;
using std::vector;
using std::cout;
using std::endl;

struct NameValuePair
{
    Utf8String Name;
    Utf8String Value;
};	

namespace
{
    char const * const DEFAULT_TEST_PATH = "/tmp";
}

class XElementTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE (XElementTest);
        CPPUNIT_TEST (ConstructorWithElementNameTest);
        CPPUNIT_TEST (ConstructorWithElementNameAndContentTest);
        CPPUNIT_TEST (AddSimpleChildTest);
        CPPUNIT_TEST (AddNullChildTest);
        CPPUNIT_TEST (AddRecursiveChildTest);
        CPPUNIT_TEST (AddChildDeleteAfterAddTest);
        CPPUNIT_TEST (BasicGetChildTest);
        CPPUNIT_TEST (EditChildInMemoryTest);
        CPPUNIT_TEST (AddNewAttributeTest);
        CPPUNIT_TEST (UpdateNewAttributeTest);
        CPPUNIT_TEST (TryGetNonExistantAttributeTest);
        CPPUNIT_TEST (TrySetEmptyAttributeName);
        CPPUNIT_TEST (LoadEmptyStringTest);
        CPPUNIT_TEST (LoadNonXmlStringTest);
        CPPUNIT_TEST (LoadIncompleteXmlStringTest);
        CPPUNIT_TEST (LoadXmlStringWithInvalidCharsTest);
        CPPUNIT_TEST (LoadValidXmlStringTest);
        CPPUNIT_TEST (LoadValidXmlWithProcessingInstructionsTest);
        CPPUNIT_TEST (LoadValidXmlWithCommentsTest);
        CPPUNIT_TEST (LoadXmlStringWithCDATATest);
        CPPUNIT_TEST (LoadXmlWithXmlEntities);
        CPPUNIT_TEST (SaveSimpleElementTest);
        CPPUNIT_TEST (SaveElementWithAttributeAndContent);
        CPPUNIT_TEST (ConstructWithInvalidName);
        CPPUNIT_TEST (SetAttributeWithInvalidName);
        CPPUNIT_TEST (SaveElementWithNestedElements);
        CPPUNIT_TEST (SaveWithEmbeddedXml);
        CPPUNIT_TEST (XDocumentTest);
        CPPUNIT_TEST (DelimitedXmlReadTest);
        CPPUNIT_TEST (BadDelimitedXmlReadTest);
    CPPUNIT_TEST_SUITE_END ();
    
    
    public:
        
        // Creating an element with just name should return a valid Xml and also valid
        // Name and Content
        void ConstructorWithElementNameTest()
        {
            Utf8String elementName = "Test";
            XElement element0(elementName);
            Utf8String result;
            
            result = element0.GetName();
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Did not return Valid Name", elementName, result);
            
            result = element0.GetContent();
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Did not return Valid content", Utf8String(""), result);
            
            bool pass = false;
            try
            {
                XElement element1("");
            }
            catch (XmlException& xe)
            {
                pass = true;
            }
            
            CPPUNIT_ASSERT_MESSAGE("Didnt throw XmlException with empty name", pass);
            
        }
        
        void ConstructorWithElementNameAndContentTest()
        {
            Utf8String elementName = "TestName";
            Utf8String elementContent = "TestContent";
            Utf8String result;
            
            XElement element0(elementName, elementContent);
            
            result = element0.GetName();
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Did not return Valid Name", elementName, result);
            
            result = element0.GetContent();
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Did not return Valid content", elementContent, result);
            
            bool pass = false;
            try
            {
                XElement element1("", elementContent);
            }
            catch (XmlException& xe)
            {
                pass = true;
            }
            
            CPPUNIT_ASSERT_MESSAGE("Didnt throw XmlException with empty name", pass);
            
            pass = true;
            try
            {
                XElement element2(elementName, "");
            }
            catch (XmlException& xe)
            {
                pass = false;
            }
            
            CPPUNIT_ASSERT_MESSAGE("Empty content should not throw", pass);
            
        }
        
        void AddSimpleChildTest()
        {
            XElement element("Test1");
            
            // Add Child 1
            XElementPtr child1(new XElement("Child1"));
            element.AddChild(child1);
            
            //Add Child 2
            XElementPtr child2(new XElement("Child2", "Content2"));
            element.AddChild(child2);
            
            vector<XElementPtr> childElements;
            element.GetChildren(childElements);
            
            bool pass1 = false;
            bool pass2 = false;
            
            Utf8String name, content;
            
            for (unsigned int i = 0; i < childElements.size(); ++i)
            {
                name = childElements[i]->GetName();
                content = childElements[i]->GetContent();
                
                if (name == "Child1")
                {
                    if (content.Empty())
                    {
                        pass1 = true;
                    }
                }
                else if (name == "Child2")
                {
                    if (content == "Content2")
                    {
                        pass2 = true;
                    }
                }
            }
            
            CPPUNIT_ASSERT_MESSAGE("Could not find Child 1", pass1);
            CPPUNIT_ASSERT_MESSAGE("Could not find Child 2", pass2);
            
        }
        
        void AddNullChildTest()
        {
            XElement element("Test1");
            
            XElementPtr test;
            
            bool pass = false;
            
            try
            {
                element.AddChild(test);
            }
            catch (XmlException&)
            {
                pass = true;
            }
            
            CPPUNIT_ASSERT(pass);
        }

                void AddRecursiveChildTest()
                {
                        XElementPtr testElem(new XElement("TestR"));
            
                        bool passTest = false;
            
                        // First, let's check the basic case:
                        try
                        {
                              testElem->AddChild(testElem);
                        }
                        catch (XmlException& x)
                        {
                            std::cout << "AddRecursiveChildTest() Base case Caught exception: " << endl; // x.xmlWhat() << endl;
                             passTest = true;
                        }

                        CPPUNIT_ASSERT(passTest);

#ifdef THIS_CODE_DEPRECATED_BECAUSE_RECURSIVE_ADD_AND_CLONING_DONT_MIX
                        passTest = false;

                        //Now let's try adding a child:
                        XElementPtr testElemLvl2(new XElement("TestLvl2"));
                        try
                        {
                             testElem->AddChild(testElemLvl2);
                             testElemLvl2->AddChild(testElem);
                        }
                        catch (XmlException& x)
                        {
                            std::cout << "AddRecursiveChildTest() Level 2: Caught exception: " << endl; // x.xmlWhat() << endl;
                             passTest = true;
                        }

                        CPPUNIT_ASSERT(passTest);
                        passTest = false;
            
                        //Now let's try to add a child to a child:
                        XElementPtr testElemLvl3(new XElement("TestLvl3"));
                        try
                        {
                             //testElem->AddChild(testElemLvl2);  //Successfully done above
                             testElemLvl2->AddChild(testElemLvl3);
                             testElemLvl3->AddChild(testElem);

                        }
                        catch (XmlException& x)
                        {
                        std::cout << "AddRecursiveChildTest() Level 3: Caught exception: " << endl; // x.xmlWhat() << endl;
                             passTest = true;
                        }

                        CPPUNIT_ASSERT(passTest);
                        passTest = false;

                        //Now let's try to adding some extra children too:
                        XElementPtr testElemLvl2a(new XElement("TestLvl2a"));
                        XElementPtr testElemLvl2b(new XElement("TestLvl2b"));
                        XElementPtr testElemLvl3a(new XElement("TestLvl3a"));
                        XElementPtr testElemLvl3b(new XElement("TestLvl3b"));
                        XElementPtr testElemLvl3c(new XElement("TestLvl3c"));
                        XElementPtr testElemLvl4(new XElement("TestLvl4"));
                        XElementPtr testElemLvl4a(new XElement("TestLvl4a"));
                        try
                        {
                            //testElem->AddChild(testElemLvl2);  //Successfully done above
                            //testElemLvl2->AddChild(testElemLvl3); //Successfully done above
                            testElemLvl2->AddChild(testElemLvl2a);
                            testElemLvl2->AddChild(testElemLvl2b);
                            testElemLvl3->AddChild(testElemLvl3a);
                            testElemLvl3->AddChild(testElemLvl3b);
                            testElemLvl3b->AddChild(testElemLvl4);
                            testElemLvl3b->AddChild(testElemLvl4a);
                            testElemLvl3->AddChild(testElemLvl3c);

                            testElemLvl4a->AddChild(testElem);   //Oh-no! Can't do that.

                        }
                        catch (XmlException& x)
                        {
                            std::cout << "AddRecursiveChildTest() Level 3 Extra: Caught exception: " << endl; // x.xmlWhat() << endl;
                        passTest = true;
                        }
            
                        CPPUNIT_ASSERT(passTest);
                        passTest = false;

                        /////Repeated Child test, no longer allowed
                        Utf8String xmlWhole;

                        try
                        {
                            std::string lvl4Name("TestLvl4");
                            XElementPtr lvl4Ptr;

                            testElemLvl3->AddChild(testElemLvl3a);

                            //This one should fail now
                            testElemLvl3b->AddChild(testElemLvl4);
                            testElemLvl3b->GetChild(lvl4Name,lvl4Ptr);
                            //std::cout << " Duplicate Child test. Name is: " << lvl4Ptr->GetName() << std::endl;

                        }
                        catch (XmlException& x)
                        {
                            std::cout << "AddRecursiveChildTest() Redundancy Test Caught exception: " << endl; // x.GetMessage() << endl;
                            passTest = true;
                        }

                        testElemLvl3->ToString(xmlWhole,false);
                        //std::cout << "Here's the whole xml: " << std::endl << xmlWhole << std::endl;

                        if (passTest && xmlWhole == "<TestLvl3><TestLvl3a></TestLvl3a><TestLvl3b><TestLvl4/><TestLvl4a/><TestLvl4/></TestLvl3b><TestLvl3c/><TestLvl3a/></TestLvl3>" )
                        {
                            std::cout << "Setting Pass5 to TRUE" << std::endl;
                            passTest = true;
                        }

                        //Finally, what's the results?
                        CPPUNIT_ASSERT(passTest);
#endif // THIS_CODE_DEPRECATED_BECAUSE_RECURSIVE_ADD_AND_CLONING_DONT_MIX
        }
        
        void AddChildDeleteAfterAddTest()
        {
            XElement element("Test");
            
            {
                // Create a scope to make the XElementPtr go out of scope and delete the pointer
                XElementPtr child(new XElement("Child"));
                
                element.AddChild(child);
            }
            
            // Verify if the child is present
            vector<XElementPtr> children;
            element.GetChildren(children);
            
            bool pass = false;
            Utf8String name;
            for (unsigned int i = 0; i < children.size(); ++i)
            {
                name = children[i]->GetName();
                
                if (name == "Child")
                {
                    pass = true;
                }
            }
            
            CPPUNIT_ASSERT(pass);
            
        }
        
        void BasicGetChildTest()
        {
            XElement element("Test");
            
            XElementPtr child1(new XElement("Child1"));
            element.AddChild(child1);
            
            XElementPtr child2(new XElement("Child2"));
            element.AddChild(child2);
            
            XElementPtr child3(new XElement("Child3"));
            element.AddChild(child3);
            
            // Boundary First
            bool pass = false;
            XElementPtr result;
            
            pass = false;
            pass = element.GetChild("Child1", result);
            CPPUNIT_ASSERT_MESSAGE("Child1 not found", pass);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Child1 Tag not correct", Utf8String("Child1"), result->GetName());
            
            // Boundary Last
            pass = false;
            pass = element.GetChild("Child3", result);
            CPPUNIT_ASSERT_MESSAGE("Child3 not found", pass);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Child3 Tag not correct", Utf8String("Child3"), result->GetName());
            
            // Middle
            pass = false;
            pass = element.GetChild("Child2", result);
            CPPUNIT_ASSERT_MESSAGE("Child2 not found", pass);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Child2 Tag not correct", Utf8String("Child2"), result->GetName());
        
            // Missing Child test
            pass = true;
            pass = element.GetChild("NotExistentChild", result);
            CPPUNIT_ASSERT_MESSAGE("NonExistentChild false positive", !pass);
        }
        
        // Validate GetChildren safe pointer by editing in place
        void EditChildInMemoryTest()
        {

                        XElement element("Test");

                        XElementPtr child1(new XElement ("Child1"));
                        element.AddChild(child1);

                        XElementPtr childReturned;

                        XElementPtr child2(new XElement ("Child2"));
                        element.GetChild("Child1", childReturned);
                        childReturned->AddChild(child2);

                        XElementPtr childReturned1, childReturned2;
                        element.GetChild("Child1", childReturned1);
                        bool pass = childReturned1->GetChild("Child2", childReturned2);
    
                        CPPUNIT_ASSERT_MESSAGE("Add child in memory failed !", pass);
            
            
            
        }
        
        void AddNewAttributeTest()
        {
            XElement element("Test");
            
            element.SetAttributeValue("Name1", "Value1");
            element.SetAttributeValue("Name2", "Value2");
            
            Utf8String value;
            bool isSet = false;
            
            isSet = element.GetAttributeValue("Name1", value);
            CPPUNIT_ASSERT_MESSAGE("Attribute 1 not present", isSet);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute 1 value not correct", Utf8String("Value1"),  value);
            
            isSet = false;
            value = "";
            isSet = element.GetAttributeValue("Name2", value);
            CPPUNIT_ASSERT_MESSAGE("Attribute 2 not present", isSet);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute 2 value not correct", Utf8String("Value2"),  value);
            
            
        }
        
        void UpdateNewAttributeTest()
        {
            XElement element("Test");
            element.SetAttributeValue("Name1", "Value1");
            
            element.SetAttributeValue("Name1", "Value2");
            Utf8String value;
            CPPUNIT_ASSERT_MESSAGE("Attribute 1 is not present", element.GetAttributeValue("Name1", value));
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute 1 value not correct", Utf8String("Value2"),  value);
        }
        
        void TryGetNonExistantAttributeTest()
        {
            XElement element("Test");
            element.SetAttributeValue("Name2", "Value2");
            Utf8String value;
            CPPUNIT_ASSERT_MESSAGE("Invalid result for Non existant attribute", !element.GetAttributeValue("Name1", value));
            
        }
        
        void TrySetEmptyAttributeName()
        {
            bool pass = false;
            XElement element("Test");
            try
            {
                element.SetAttributeValue("", "");
            }
            catch (XmlException&)
            {
                pass = true;
            }
            
            CPPUNIT_ASSERT(pass);
            
        }
        
        void LoadStringAndPassAtException(const string xmlString)
        {
            bool pass = false;
            
            try
            {
                XElementPtr element(new XElement("Root"));
                XElement::Load(xmlString, element);
            }
            catch (XmlException&)
            {
                pass = true;
            }
            CPPUNIT_ASSERT(pass);
        }
        
        // Loading an empty string should through Exception
        void LoadEmptyStringTest()
        {
            LoadStringAndPassAtException("");
        }
        
        
        // Loading a non xml string should through an exception
        void LoadNonXmlStringTest()
        {
            LoadStringAndPassAtException("THIS IS NOT A XML STRING");
        }
        
        // Load Incomplete Xml String and it should though XmlException
        void LoadIncompleteXmlStringTest()
        {
            LoadStringAndPassAtException("<Test>");
            
            LoadStringAndPassAtException("<Test ada=\"\"><Test1><Test2></Test3></Test1></Test>");
        }
        
        // Load Xml with invalid characters and it should though XmlException
        void LoadXmlStringWithInvalidCharsTest()
        {
            LoadStringAndPassAtException("<Test ada=\"\"><Test1><Test2ad@#$%^&*()_)\\//></Test3></Test1></Test>");
        }
        
        
        void ValidateAttributes(XElementPtr element, NameValuePair* pairs, int count)
        {
            Utf8String attribValue;
            bool pass = false;
            char message[100];
            for (int i = 0; i < count; ++i)
            {
                pass = false;
                pass = element->GetAttributeValue(pairs[i].Name, attribValue);
                sprintf(message, "Attribute %d of Child %s not found", i, element->GetName().Str().c_str());
                CPPUNIT_ASSERT_MESSAGE(message, pass);
                
                sprintf(message, "Attribute %d value of Child %s not correct", i, element->GetName().Str().c_str());
                CPPUNIT_ASSERT_EQUAL_MESSAGE(message, pairs[i].Value, attribValue);
            }
        }
        
        // Load a Simple Xml string and validate the parse
        void LoadValidXmlStringTest()
        {
            bool pass = false;
            string attributeValue, content;
            string xmlStringIn = "<Test Name0=\"val0\"><Test1 Name1=\"Val1\" Name2=\"Val2\"/><Test3 Name3=\"Val3\">Content1<Test4 name4=\"val4\"><Test5/></Test4></Test3></Test>";
            string xmlStringOut = "<Test Name0=\"val0\"><Test1 Name1=\"Val1\" Name2=\"Val2\"/><Test3 Name3=\"Val3\">Content1<Test4 name4=\"val4\"><Test5/></Test4></Test3></Test>";
            
            XElementPtr root;
            XElement::Load(xmlStringIn, root);
            
            CPPUNIT_ASSERT_MESSAGE("Parsed element is null", (root != NULL));
            
            // Root element tests
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Root Element name not valid", Utf8String("Test"), root->GetName());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Root Element content not valid", Utf8String(""), root->GetContent());
            
            NameValuePair rootvalues[1] = {
                                            {"Name0", "val0"},
                                         };
            
            ValidateAttributes(root, rootvalues, 1);
            
            // Child "Test1" tests
            NameValuePair test1values[2] = {
                                            {"Name1", "Val1"},
                                            {"Name2", "Val2"},
                                         };
            
            XElementPtr test1;
            pass = false;
            pass = root->GetChild("Test1", test1);
            CPPUNIT_ASSERT_MESSAGE("Child Test1 not found", pass);
            ValidateAttributes(test1, test1values, 2);
            
            // Child Test3 tests
            XElementPtr test3;
            pass = false;
            pass = root->GetChild("Test3", test3);
            CPPUNIT_ASSERT_MESSAGE("Child Test3 not found", pass);
            
            NameValuePair test3values[1] = {
                                            {"Name3", "Val3"},
                                         };
            ValidateAttributes(test3, test3values, 1);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Test3 Element content not valid", Utf8String("Content1"), test3->GetContent());
            
            // Child Test4 tests
            XElementPtr test4;
            pass = false;
            pass = test3->GetChild("Test4", test4);
            CPPUNIT_ASSERT_MESSAGE("Child Test4 not found", pass);
            
            NameValuePair test4values[1] = {
                                            {"name4", "val4"},
                                         };
            ValidateAttributes(test4, test4values, 1);
            
            // Child Test5 tests
            XElementPtr test5;
            pass = false;
            pass = test4->GetChild("Test5", test5);
            CPPUNIT_ASSERT_MESSAGE("Child Test5 not found", pass);
            
        }
        
        void LoadValidNonAsciiXmlStringTest()
        {
            bool pass = false;
            string attributeValue, content;
            Utf8String xmlStringIn("<Test Name0=\"val0\">"
                                    "<Test1 Name1=\"Val1\" Name2=\"Val2\"/>"
                                    "<Test3 Name3=\"Jos" "\xC3" "\xA9" " Garc" "\xC3" "\xAD" "a\">"
                                     "Contenido \xC3" "\xBA" "nico"
                                     "<Test4 name4=\"val4\">"
                                      "<Test5/>"
                                     "</Test4>"
                                    "</Test3>"
                                   "</Test>");
            Utf8String xmlStringOut("<Test Name0=\"val0\"><Test1 Name1=\"Val1\" Name2=\"Val2\"/>"
                                    "<Test1 Name1=\"Val1\" Name2=\"Val2\"/>"
                                    "<Test3 Name3=\"Jos" "\xC3" "\xA9" " Garc" "\xC3" "\xAD" "a\">"
                                     "Contenido \xC3" "\xBA" "nico"
                                     "<Test4 name4=\"val4\">"
                                      "<Test5/>"
                                     "</Test4>"
                                    "</Test3>"
                                   "</Test>");
            
            XElementPtr root;
            XElement::Load(xmlStringIn, root);
            
            CPPUNIT_ASSERT_MESSAGE("Parsed element is null", (root != NULL));
            
            // Root element tests
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Root Element name not valid", Utf8String("Test"), root->GetName());
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Root Element content not valid", Utf8String(""), root->GetContent());
            
            NameValuePair rootvalues[1] = {
                                            {"Name0", "val0"},
                                         };
            
            ValidateAttributes(root, rootvalues, 1);
            
            // Child "Test1" tests
            NameValuePair test1values[2] = {
                                            {"Name1", "Val1"},
                                            {"Name2", "Val2"},
                                         };
            
            XElementPtr test1;
            pass = false;
            pass = root->GetChild("Test1", test1);
            CPPUNIT_ASSERT_MESSAGE("Child Test1 not found", pass);
            ValidateAttributes(test1, test1values, 2);
            
            // Child Test3 tests
            XElementPtr test3;
            pass = false;
            pass = root->GetChild("Test3", test3);
            CPPUNIT_ASSERT_MESSAGE("Child Test3 not found", pass);
            
            NameValuePair test3values[1] = {
                                            {"Name3", "Val3"},
                                         };
            ValidateAttributes(test3, test3values, 1);
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Test3 Element content not valid", Utf8String("Content1"), test3->GetContent());
            
            // Child Test4 tests
            XElementPtr test4;
            pass = false;
            pass = test3->GetChild("Test4", test4);
            CPPUNIT_ASSERT_MESSAGE("Child Test4 not found", pass);
            
            NameValuePair test4values[1] = {
                                            {"name4", "val4"},
                                         };
            ValidateAttributes(test4, test4values, 1);
            
            // Child Test5 tests
            XElementPtr test5;
            pass = false;
            pass = test4->GetChild("Test5", test5);
            CPPUNIT_ASSERT_MESSAGE("Child Test5 not found", pass);
            
        }
        
        void LoadXmlStringWithCDATATest()
        {
            string xmlString = "<Test><![CDATA[&&***#4<>EAE!@?/\\<TEMP/>]]></Test>";
            XElementPtr element;
            XElement::Load(xmlString, element);
            CPPUNIT_ASSERT_EQUAL(Utf8String("&&***#4<>EAE!@?/\\<TEMP/>"), element->GetContent());
        }
        
        
        // Loading a xml string with processing instruction
        // should ingore it
        void LoadValidXmlWithProcessingInstructionsTest()
        {
            XElementPtr element;
            XElement::Load("<?xml version=\"1.0\" ?> <TestRequest/>", element);
            
                        // JWF -- The underlying layer is handling processing instructions, and the element
                        //        containing that information is on top of the stack.  
            // CPPUNIT_ASSERT_EQUAL(string("?xml"), element->GetName());
            CPPUNIT_ASSERT(Utf8String("TestRequest")== element->GetName());
        }
        
        // Loading a valid Xml with comment line should ignore it
        void LoadValidXmlWithCommentsTest()
        {
            XElementPtr element;
            XElement::Load("<!-- This is comment line --><TestRequest/>", element);
            
            CPPUNIT_ASSERT(Utf8String("TestRequest") == element->GetName());
        }
        
        void LoadXmlWithXmlEntities()
        {
            try {
            XElementPtr element;
            XElement::Load("<Test>&quot;&amp;&apos;&lt;&gt;&quot;</Test>", element);
            
            CPPUNIT_ASSERT(Utf8String("\"&'<>\"") == element->GetContent());
            } catch (SCXCoreLib::SCXException& e)
            {
                printf("SCX e = %ls", e.What().c_str());
            }

        }
        void MultiThreadedLoadTest()
        {
        }
        
        void SaveSimpleElementTest()
        {
            XElement element("Test");
            Utf8String xmlString;
            
            element.ToString(xmlString, false);
            
            CPPUNIT_ASSERT(Utf8String("<Test/>") == xmlString);
        }
        
        void PrintHex(const string& s)
        {
            const char* str = s.c_str();
            while (*str)
            {
                printf("%x ", *str);
                str++;
            }
            printf("\n");
        }
        
        void SaveElementWithAttributeAndContent()
        {
            XElement element("Test", "Content");
            Utf8String xmlString;
            
            element.SetAttributeValue("Name1", "Value1");
            element.SetAttributeValue("Name3", "Value2");
            element.ToString(xmlString, true);
            
            Utf8String expected("<Test Name1=\"Value1\" Name3=\"Value2\">Content</Test>\r\n");
            
            /*
            printf("\n");
            PrintHex(expected);
            PrintHex(xmlString);
            */
            
            CPPUNIT_ASSERT(expected == xmlString);
        }
        
        void ConstructAndExpectFail(const char* failMsg, string name, string content, string attributeName = "", string attributeValue = "")
        {
            bool pass = false;
            try
            {
                XElement element(name, content);
                if (!attributeName.empty())
                {
                    element.SetAttributeValue(attributeName, attributeValue);
                }
            }
            catch (XmlException&)
            {
                pass = true;
            }
            CPPUNIT_ASSERT_MESSAGE(failMsg, pass);

            
        }
        void ConstructWithInvalidName()
        {
#if !defined(hpux)      // remove this when bug 437642, XElement bad name/attr. char. exception is not being propagated on HP-UX, is resolved
            // Starts with :
            ConstructAndExpectFail("Name starts with : should fail", ":name", "content");
            
            // Contains non printable character(s): in US ASCII, 0xC2 and 0x8D are non-ASCII
            // because they have the eighth bit set.  In ISO 8859-x encodings, the string is
            // "n, a, A-circumflex, RI, m, e".  The RI is the reverse-index C1 control character,
            // which is non-printable.  In the UTF-8 encoding, the string is "n, a, RI, m, e".
            ConstructAndExpectFail("containing non printable should fail", "na\xC2\x8Dme", "content");
            
            ConstructAndExpectFail("names with space should fail", "na me", "content");
            
            ConstructAndExpectFail("name with xml entities should fail", "na&\"me", "content");
#endif
            return;
        }
        
        void SetAttributeWithInvalidName()
        {
#if !defined(hpux)      // remove this when bug 437642, XElement bad name/attr. char. exception is not being propagated on HP-UX, is resolved
            // Starts with :
            ConstructAndExpectFail("Name starts with : should fail", "name", "content", ":name", "content");
            
            // Contains non printable character(s)--see comments above.
            ConstructAndExpectFail("containing non printable should fail", "name", "content", "na\xC2\x8Dme", "content");
            
            ConstructAndExpectFail("names with space should fail", "name", "content", "na me", "content");
            
            ConstructAndExpectFail("name with xml entities should fail", "name", "content", "na&\"me", "content");
#endif
            return;
        }
        
        void SaveElementWithNestedElements()
        {
            XElementPtr root(new XElement("Test1", "Content1"));
            XElementPtr child1(new XElement("Child1", "Content1"));
            XElementPtr child2(new XElement("Child2", "Content2"));
            XElementPtr child3(new XElement("Child1", ""));
            
            child1->SetAttributeValue("Name1", "Value1");
            child2->SetAttributeValue("Name2", "Value2");
            child1->AddChild(child2);
            
            root->AddChild(child1);
            root->AddChild(child3);
            
            
            Utf8String xmlstring;
            root->ToString(xmlstring, true);
            
//            static std::string c_UTF8_BOM_STRING = "\x00EF\x00BB\x00BF";
//            Utf8String expected(c_UTF8_BOM_STRING);
            Utf8String expected;
            expected += "<Test1>\r\nContent1    <Child1 Name1=\"Value1\">\r\nContent1        <Child2 Name2=\"Value2\">Content2</Child2>\r\n</Child1>\r\n    <Child1/>\r\n</Test1>\r\n";
            
            /*
            printf("\n");
            PrintHex(expected);
            PrintHex(xmlstring);
            */
            CPPUNIT_ASSERT(expected == xmlstring);
        }
        
        void SaveWithEmbeddedXml()
        {
            XElement element("Test", "<test>\"'&</test>");
            Utf8String xmlString;
            element.ToString(xmlString, false);
            
            Utf8String expected = "<Test>&lt;test&gt;&quot;&apos;&amp;&lt;/test&gt;</Test>";
            
            /*
            printf("\n");
            PrintHex(expected);
            PrintHex(xmlString);
            */
            
            CPPUNIT_ASSERT(expected == xmlString);
        }
        
        void XDocumentTest()
        {
            // Get the place where the test XML files will be written
            char const* Dir = getenv("CM_HOME");

            if (Dir == NULL)
            {
                Dir = DEFAULT_TEST_PATH;
            }
            std::string TestDir(Dir);
            TestDir += "/tmp";
            (void)mkdir(TestDir.c_str(), 0755);

            // create a small XML document
            XDocument TestDoc;
            try
            {
                // Create the root elements and give them some attributes
                XElementPtr RootElement1(new XElement("RootElement", "Root Content"));
                RootElement1->SetAttributeValue("RootAttr1", "Root Attr 1 Value");
                RootElement1->SetAttributeValue("RootAttr2", "Root Attr 2 Value");

                // Create three child elements with some attributes and data
                XElementPtr ChildElement1(new XElement("Child1", "Child 1 Content"));
                ChildElement1->SetAttributeValue("ChildAttr11", "Child Attr 11 Value");
                ChildElement1->SetAttributeValue("ChildAttr12", "Child Attr 12 Value");
                ChildElement1->SetAttributeValue("ChildAttr13", "Child Attr 13 Value");
                XElementPtr ChildElement2(new XElement("Child2", "Child 2 Content"));
                ChildElement2->SetAttributeValue("ChildAttr21", "Child Attr 21 Value");
                ChildElement2->SetAttributeValue("ChildAttr22", "Child Attr 22 Value");
                XElementPtr ChildElement3(new XElement("Child3"));
                ChildElement3->SetAttributeValue("ChildAttr31", "Child Attr 31 Value");
                ChildElement3->SetAttributeValue("ChildAttr32", "Child Attr 32 Value");

                // Create a grandchild element with some attributes
                XElementPtr ChildElement31(new XElement("Child31", "Child 31 Content"));
                ChildElement31->SetAttributeValue("ChildAttr311", "Child Attr 311 Value");
                ChildElement31->SetAttributeValue("ChildAttr312", "Child Attr 312 Value");

                // Create two great-grandchild elements with some attributes and data
                XElementPtr ChildElement311(new XElement("Child311", "Child 311 Content"));
                XElementPtr ChildElement312(new XElement("Child312", "Child 312 Content"));
                ChildElement312->SetAttributeValue("ChildAttr3121", "Child Attr 3121 Value");
                ChildElement312->SetAttributeValue("ChildAttr3122", "Child Attr 3122 Value");

                // Add the great-grandchild elements to a grandchildchild element
                ChildElement31->AddChild(ChildElement311);
                ChildElement31->AddChild(ChildElement312);

                // Add the grandchild element to a child element
                ChildElement3->AddChild(ChildElement31);

                // Add the child elements to the root element
                RootElement1->AddChild(ChildElement1);
                RootElement1->AddChild(ChildElement2);
                RootElement1->AddChild(ChildElement3);

                // Create the document with the two root elements and their descendents
                TestDoc.SetRootElement(RootElement1);
                TestDoc.SetComment("Root comment");
                TestDoc.SetDocumentType("RootElement[]");
            }
            catch (XmlException e)
            {
                CPPUNIT_FAIL("Error in creating XML file");
            }

            // Save the document
            std::string TestFileName(TestDir + "/TestDoc.xml");
            try
            {
                TestDoc.Save(TestFileName);
            }
            catch (XmlException e)
            {
                CPPUNIT_FAIL("Error saving XML file");
            }

            // Check the file for known contents.
            std::string GrepCommand("grep '<Child312 ChildAttr3121=\"Child&#x0020;Attr&#x0020;3121&#x0020;Value\" ChildAttr3122=\"Child&#x0020;Attr&#x0020;3122&#x0020;Value\"' " + TestFileName + "> /dev/null");
            if (system(GrepCommand.c_str()) != 0)
            {
                CPPUNIT_FAIL("Error in XML output file");
            }

            // Load a new document from the file just saved
            XDocument NewDoc;
            try
            {
                XDocument::LoadFile(TestFileName, NewDoc);
            }
            catch (XmlException e)
            {
                CPPUNIT_FAIL("Error loading XML file");
            }

            // Save the new document
            std::string TestFileName2(TestDir + "/TestDoc2.xml");
            TestDoc.Save(TestFileName2);

            std::string DiffCommand = "diff -b " + TestFileName + " " + TestFileName2;
            if (system(DiffCommand.c_str()) != 0)
            {
                CPPUNIT_FAIL("Store / Load / Store produced different XML files");
            }

            return;
        }

        void DelimitedXmlReadTest()
        {
            Utf8String xmlString = "<Condition>\r\n";
            xmlString += "<Expression ExpressionType=\"until-true\" ExpressionLanguage=\"WQL\">\r\n";
            xmlString += "@root\\ccm&#x0D;&#10;SELECT * FROM SMS_Client WHERE ClientVersion &gt;= &quot;4.00.5300.0000&quot;&#13;&#x0A;\r\n";
            xmlString += "</Expression>\r\n";
            xmlString += "</Condition>\r\n";

            Utf8String expectedContent = "@root\\ccm\r\nSELECT * FROM SMS_Client WHERE ClientVersion >= \"4.00.5300.0000\"\r\n\r\n";

            XElementPtr root, child;
            XElement::Load(xmlString, root);

            CPPUNIT_ASSERT(root->GetChild("Expression", child));

            Utf8String content = child->GetContent();
            
            CPPUNIT_ASSERT_EQUAL(expectedContent.Size(), content.Size());

            /*
            for (int i = 0; i < expectedContent.Size(); ++i)
            {
                printf(" %d %d - %s\n", expectedContent[i].GetCodePoint(), content[i].GetCodePoint(), (expectedContent[i].GetCodePoint() != content[i].GetCodePoint())?"***": "");
            }
            */
            CPPUNIT_ASSERT_EQUAL(expectedContent, content);

        }

        void BadDelimitedXmlReadTest()
        {
            Utf8String xmlString = "<Condition>\r\n";
            xmlString += "<Expression ExpressionType=\"until-true\" ExpressionLanguage=\"WQL\">\r\n";
            xmlString += "@root\\ccm&#x13;&#ZOO;&#10;SELECT * FROM SMS_Client WHERE ClientVersion &gt;= &quot;4.00.5300.0000&quot;&#13;&#x10;\r\n";
            xmlString += "</Expression>\r\n";
            xmlString += "</Condition>\r\n";

            XElementPtr root, child;
            try
            {
                XElement::Load(xmlString, root);
                CPPUNIT_FAIL("No XmlException");
            } catch (XmlException& xe)
            {}
            catch (...)
            {
                CPPUNIT_FAIL("No XmlException");
            }
                

        }

};

CPPUNIT_TEST_SUITE_REGISTRATION (XElementTest);
