#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include "opencv2/objdetect.hpp"

#include "opencv2/videoio.hpp"

using namespace std;
using namespace cv;

void detectAndDraw( Mat frame );
CascadeClassifier face_cascade;
CascadeClassifier eyes_cascade;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cout << "\tThis is a demo program to show how an opencv applicstion can be run in parallel threads.\n";
        cout << "\tThe example is taken from official doccumentation at:\n";
        cout << "\n\t\thttps://docs.opencv.org/3.4/db/d28/tutorial_cascade_classifier.html\n\n";
        cout << "\tAs describes in the documentation, the program needs two xml files containing training data.\n";
        cout << "\tThe documentation refers to the files, but for convenience these two files are copied in:\n";
        cout << "\n\t\tproject_repository/input_files/haarcascades/\n\n";
        cout << "\tIf you process images from a different location, please copy the directory to the same placeas your images.\n\n";
        cout << "\tThis program should be started with one command line parameter, the source directory:\n";
        cout << "\n\t\tcascade_classifier path/to/your/source/images/\n";
        cout << "\tIt will process all images in the given director, find faces and eyes and draw\n";
        cout << "\tcircles around them and then save them in a destination directory with the path\n";
        cout << "\n\t\t./your_last_processed_images_singlethread/\n\n";
        cout << "\twith the same name as in the source directory with \'_modified\' added to the name.\n";
        cout << "\tIf ./your_last_processed_images_singlethread/ already exists, the new files will be added.\n";
        cout << "\tPossible previous files with the same name will be overwritten without warning.\n\n";

        return 0;
    }

     // Check the input location, find the processable files and store their path in a vector.
    // For each input file create a corresponding output file name with _modified added to the stem.
    // Also, check if the destination location is empty, or there are files from previous run.
    // In the latter case, inform the used and ask for confirmation before start.

    string sourcePath = argv[1];
    string destinationPath = "./your_last_processed_images_singlethread/";
    vector<filesystem::path> sourceFiles;
    vector<filesystem::path> destinationFiles;
    if (filesystem::is_directory(sourcePath))
    {
        for (const filesystem::directory_entry & entry : filesystem::directory_iterator(sourcePath))
        {
            if (cv::haveImageReader (entry.path())) 
            {
                filesystem::path p = entry.path();
                sourceFiles.emplace_back(p);
                destinationFiles.emplace_back(filesystem::path(destinationPath + (string) p.stem() + "_modified" + (string) p.extension()));
            }
        }
        if (sourceFiles.size() > 0)
        {
            cout << sourceFiles.size() << " files will be processed.\n";

            for (size_t i = 0; i < sourceFiles.size(); i++)
            {
                cout << i << ") " << sourceFiles[i] << " -> " << destinationFiles[i] << "\n";
            }
        }
        else
        {
            cout << "No processable files were found. \n";
            return 0;
        }
    }

    if (filesystem::is_directory(destinationPath))
    {
        size_t nFiles = count_if(filesystem::directory_iterator(destinationPath), filesystem::directory_iterator(), []( const std::filesystem::directory_entry & entry){return entry.is_regular_file();});
        if (nFiles > 0) 
        {
            cout << "The destination path exists with "<< nFiles << " files.\n";
            int fileCounter = 0;
            for (const filesystem::directory_entry & entry : filesystem::directory_iterator(destinationPath))
            {
                if (find_if(destinationFiles.begin(), destinationFiles.end(), [entry](filesystem::path & p) {return entry.path().filename() == p.filename();})!= destinationFiles.end())
                {
                    cout << ++fileCounter << ") " << entry.path() << " will be replaced.\n";
                }
            }
           if (fileCounter == 0)
           {
               cout << "No files will be overwritten.\n";
           }
        }
        else
        {
            cout << "The destination path exists but without any regular files.\n";
        }
        cout << destinationPath << " will be used for output files.\n";
    }
    else
    {
        cout << destinationPath << " does not exist. A new directory is created.\n";
        filesystem::create_directories(destinationPath);
    }
 
    // Load the training data for detection of faces and eyes. 
    String face_cascade_name = sourcePath+"/haarcascades/haarcascade_frontalface_alt.xml";
    String eyes_cascade_name = sourcePath+"/haarcascades/haarcascade_eye_tree_eyeglasses.xml";
    //-- 1. Load the cascades
    if( !face_cascade.load( face_cascade_name ) )
    {
        cout << "--(!)Error loading face cascade\n";
        return -1;
    };
    if( !eyes_cascade.load( eyes_cascade_name ) )
    {
        cout << "--(!)Error loading eyes cascade\n";
        return -1;
    };
    cout << "\n\n\nIf you are fine with processing of the file as described above, respond with Yes or yes! \n\n";

    cout << ">> ";
    string r;
    cin >> r;
    cout << "You answered \'" << r << "\'.\n";
    if (!((r == "Yes") || (r == "yes"))) return 0;
    cout << "\n\n\nStart processing...\n\n\n";

    // Start processing after the final confirmation.

    for (size_t index = 0; index < sourceFiles.size(); index++)
    {
        Mat frame = imread((string) sourceFiles[index],IMREAD_COLOR);
        detectAndDraw( frame );
        imwrite(destinationFiles[index], frame);
    }
}
void detectAndDraw( Mat frame )
{
    Mat frame_gray;
    cvtColor( frame, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );         
    //-- Detect faces
    std::vector<Rect> faces;
    face_cascade.detectMultiScale( frame_gray, faces );
    for ( size_t i = 0; i < faces.size(); i++ )
    {
        Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
        ellipse( frame, center, Size( faces[i].width/2, faces[i].height/2 ), 0, 0, 360, Scalar( 255, 0, 255 ), 4 );
        Mat faceROI = frame_gray( faces[i] );
        //-- In each face, detect eyes
        std::vector<Rect> eyes;
        eyes_cascade.detectMultiScale( faceROI, eyes );
        for ( size_t j = 0; j < eyes.size(); j++ )
        {
            Point eye_center( faces[i].x + eyes[j].x + eyes[j].width/2, faces[i].y + eyes[j].y + eyes[j].height/2 );
            int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
            circle( frame, eye_center, radius, Scalar( 255, 0, 0 ), 4 );
        }
    }
}
