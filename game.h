#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include<cmath>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // For transformations like translate, rotate, scale
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

const glm::vec3 GRAVITY(0.0f, -70.0f, 0.0f);

glm::mat4 view          = glm::mat4(1.0f);
glm::mat4 projection    = glm::mat4(1.0f);

//struct that stores all the stuffs needed to implement physics
struct physicsComponent{
    float mass = 1.0;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); //object center
    glm::vec3 velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

    //For Gravity:
    bool hasGravity = false;
    bool onGround = false;

    //For Collision:
    bool hasCollision = false;
    float coeffOfRestitution = 1.0;
    bool isStatic = false;

    //AABB Collision
    bool isAABB = false;
    std::vector<float> boundary = {0, 0, 0, 0, 0, 0};
    //spherical collision
    bool isSpherical = false;
    float radius = 0.0f;
};

/* struct used in collision system to know the if collision has occured,
the overlap between the colliding objects and the collision normal */
struct collided{
    bool hasCollided = false;
    float overlap = 0.0f;
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f);
};

// camera class which stores and updates the camera system
class camera{
    private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    glm::mat4 lookAtMatrix;

    public:
    camera()
    {
        this->position = glm::vec3(0.0f,0.0f,0.0f);
        this->target = glm::vec3(0.0f,0.0f,0.0f);
        this->up = glm::vec3(0.0f,0.0f,0.0f);
    }

    camera(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    {
        this->position = position;
        this->target = target;
        this->up = up;
    }

    void setCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up)
    {
        this->position = position;
        this->target = target;
        this->up = up;
    }

    glm::mat4 viewMatrix()
    {
        lookAtMatrix = glm::lookAt(position, target, up);
        return lookAtMatrix;
    }

    void updateCameraPosition(glm::vec3 position)
    {
        this->position = position;
    }

    void updateCameraPosition(glm::mat4 matrix)
    {
        glm::vec4 temp(position, 1.0f);
        temp = temp * matrix;
        position = glm::vec3(temp.x, temp.y, temp.z);
    }

    void updateTargetPosition(glm::vec3 target)
    {
        this->target = target;
    }

    void updateTargetPosition(glm::mat4 matrix)
    {
        glm::vec4 temp(target, 1.0f);
        temp = temp * matrix;
        target = glm::vec3(temp.x, temp.y, temp.z);
    }
};

//model class which stores the vertices, faces, normal, color of a model and renders them
class model{
    private:
    std::vector<float>vertices;
    std::vector<int>faces;
    std::vector<float>vertexNormals;
    std::ifstream file;
    shader *modelShader;
    glm::vec4 color;
    unsigned int VBO_position, VBO_normal, VAO, EBO;

    //std::vector<float>vertexTestures;
    public:
    model()
    {
        color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    void attachBuffers()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO_position);
        glGenBuffers(1, &EBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(int), faces.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0); 

        // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0);
    }

    void loadModel(const std::string &name) //loading any .obj files
    {
        std::string line;
        file.open(name.c_str());

        if(file.is_open())
        {
            while(getline(file, line))
            {
                
                if(line[0] == 'v' && line[1] == ' ')
                {
                    float x, y, z;
                    sscanf(line.c_str(), "v %f %f %f", &x, &y, &z);
                    vertices.push_back(x/2);
                    vertices.push_back(y/2);
                    vertices.push_back(z/2);
                }

                if(line[0] == 'f' && line[1] == ' ')
                {
                    int x, y, z;
                    int a, b, c, p, q, r;
                    sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &x, &a, &p, &y, &b, &q, &z, &c, &r);
                    faces.push_back(x-1);
                    faces.push_back(y-1);
                    faces.push_back(z-1);
                }
            }
        }

        attachBuffers(); 
    }
    
    void block2D(float length, float breadth)
    {
        vertices.clear();
        faces.clear();

        vertices = {
            length/2, breadth/2, -1,
            -length/2, breadth/2, -1,
            length/2, -breadth/2, -1,
            -length/2, -breadth/2, -1
        };

        faces = {
            0, 1, 2,
            1, 2, 3
        };

        attachBuffers();
    }

    void block3D(float length, float breadth, float width)
    {
        vertices.clear();
        faces.clear();

        vertices = {
            length/2, breadth/2, 0,
            -length/2, breadth/2, 0,
            length/2, -breadth/2, 0,
            -length/2, -breadth/2, 0,

            length/2, breadth/2, width,
            -length/2, breadth/2, width,
            length/2, -breadth/2, width,
            -length/2, -breadth/2, width
        };

        faces = {
            // Back face
            0, 1, 2,
            2, 3, 0,
            // Front face
            4, 5, 6,
            6, 7, 4,
            // Left face
            4, 0, 3,
            3, 7, 4,
            // Right face
            1, 5, 6,
            6, 2, 1,
            // Bottom face
            4, 5, 1,
            1, 0, 4,
            // Top face
            3, 2, 6,
            6, 7, 3
        };

        attachBuffers();
    }

    void circle2D(float radius)
    {
        vertices.clear();
        faces.clear();

        float theta;
        int segments = 50;

        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(-1.0f);

        for(int i = 0; i < segments; i++)
        {
            theta = 2 * 3.1415 * i / segments;
            vertices.push_back(radius * cos(theta));
            vertices.push_back(radius * sin(theta));
            vertices.push_back(-1.0f);

            faces.push_back(i+1);
        }

        attachBuffers();
        std::cout << "circle2D called, vertices: " << vertices.size() << "\n";
    }

    void calculateNormals()
    {
        vertexNormals.resize(vertices.size(), 0.0f);

        for(int i = 0, size = faces.size()/3; i < size; i++)
        {
            //retrieving face indices
            int f1, f2, f3;
            f1 = faces[3 * i + 0];
            f2 = faces[3 * i + 1];
            f3 = faces[3 * i + 2];

            //getting vertex data from the given indices
            glm::vec3 v1(vertices[3 * f1 + 0], vertices[3 * f1 + 1], vertices[3 * f1 + 2]);
            glm::vec3 v2(vertices[3 * f2 + 0], vertices[3 * f2 + 1], vertices[3 * f2 + 2]);
            glm::vec3 v3(vertices[3 * f3 + 0], vertices[3 * f3 + 1], vertices[3 * f3 + 2]);

            //calculate normal
            glm::vec3 normal = glm::normalize(glm::cross((v2 - v1), (v3 - v1)));

            
            vertexNormals[3 * f1 + 0] += normal.x;
            vertexNormals[3 * f1 + 1] += normal.y;
            vertexNormals[3 * f1 + 2] += normal.z;

            vertexNormals[3 * f2 + 0] += normal.x;
            vertexNormals[3 * f2 + 1] += normal.y;
            vertexNormals[3 * f2 + 2] += normal.z;

            vertexNormals[3 * f3 + 0] += normal.x;
            vertexNormals[3 * f3 + 1] += normal.y;
            vertexNormals[3 * f3 + 2] += normal.z;
            
        }
        for (int i = 0; i < vertexNormals.size(); i += 3) {
            glm::vec3 norm(vertexNormals[i], vertexNormals[i+1], vertexNormals[i+2]);
            norm = glm::normalize(norm);
            vertexNormals[i] = norm.x;
            vertexNormals[i+1] = norm.y;
            vertexNormals[i+2] = norm.z;
        }

        glGenBuffers(1, &VBO_normal);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
        glBufferData(GL_ARRAY_BUFFER, vertexNormals.size() * sizeof(float), vertexNormals.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
    }

    //set functions to set different values in the class
    void setShader(shader *modelShader)
    {
        this->modelShader = modelShader;
    }

    void setColor(glm::vec4 color)
    {
        this->color = color;
    }

    //get function to get different values from the class
    float getAverageVertices()
    {
        float average = 0;
        int vSize = vertices.size() / 3;

        for(int i = 0; i < vSize; i++)
        {
            average += abs(vertices[3 * i]);
        }

        return average/vSize;
    }

    std::vector<float> getBoundary()
    {
        int size = vertices.size()/3;

        float minX = vertices[0];
        float maxX = vertices[0];

        float minY = vertices[1];
        float maxY = vertices[1];

        float minZ = vertices[2];
        float maxZ = vertices[2];

        for(int i = 1; i < size; i++)
        {
            if(vertices[3 * i + 0] < minX)
                minX = vertices[3 * i + 0];

            if(vertices[3 * i + 1] < minY)
                minY = vertices[3 * i + 1];

            if(vertices[3 * i + 2] < minZ)
                minZ = vertices[3 * i + 2];

            if(vertices[3 * i + 0] > maxX)
                maxX = vertices[3 * i + 0];

            if(vertices[3 * i + 1] > maxY)
                maxY = vertices[3 * i + 1];

            if(vertices[3 * i + 2] > maxZ)
                maxZ = vertices[3 * i + 2];
        }

        return {minX, maxX, minY, maxY, minZ, maxZ};
    }

    //functions for rendering
    void draw(bool isCircle)
    {
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        if(isCircle)
            glDrawElements(GL_TRIANGLE_FAN, faces.size(), GL_UNSIGNED_INT, 0);
        else
            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time 
    }

    void useShader(glm::mat4 model)
    {
        modelShader->use();

        modelShader->setMat4("model", model);
        modelShader->setMat4("view", view);
        modelShader->setMat4("projection", projection);
        modelShader->setVec4("color", color);
    }

    ~model()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO_position);
        glDeleteBuffers(1, &EBO);
    }
};

class gameObject{
    protected:
    model object;
    physicsComponent physics;
    glm::mat4 objTranslation; // translation caused by phyics
    glm::mat4 model; // translation done by user
    bool isCircle; // 0 means block 1 means circle

    public:
    gameObject()
    {
        initialize();
    }

    gameObject(shader* mainShader)
    {      
        object.setShader(mainShader);
        initialize();
    }

    gameObject(std::string filepath)
    {
        object.loadModel(filepath.c_str());
        // object.calculateNormals();
        initialize();
    }

    void initialize()
    {
        objTranslation = glm::mat4(1.0f);
        model = glm::mat4(1.0f);
    }

    void block2D(float length, float breadth)
    {
        isCircle = false;
        object.block2D(length, breadth);
        physics.boundary = object.getBoundary();
    }

    void block3D(float length, float breadth, float width)
    {
        isCircle = false;
        object.block3D(length, breadth, width);
        physics.boundary = object.getBoundary();
    }
    
    void circle2D(float radius)
    {
        isCircle = true;
        object.circle2D(radius);
        physics.radius = radius;
        physics.boundary = {-radius, radius, -radius, radius, -1, -1};
    }

    float applyTransformToFloat(float coordinate, char axis='x')
    {
        if(axis == 'x' || axis == 'X')
            return glm::vec4(model * objTranslation * glm::vec4(coordinate, 0.0f, 0.0f, 1.0f)).x;

        if(axis == 'y' || axis == 'Y')
            return glm::vec4(model * objTranslation * glm::vec4(0.0f, coordinate, 0.0f, 1.0f)).y;

        if(axis == 'z' || axis == 'Z')
            return glm::vec4(model * objTranslation * glm::vec4(0.0f, 0.0f, coordinate, 0.0f)).z;

        return 0.0f;
    }

    collided checkAABBCollision(gameObject *objPtr)
    {
        if (!this->getCollisionStatus() || !objPtr->getCollisionStatus())
        {
            return {false, 0.0f, glm::vec3(0.0f)};
        }
    
        std::vector<float> objBoundary = objPtr->getBoundary();
    
        float thisLeft = applyTransformToFloat(physics.boundary[0], 'X');
        float thisRight = applyTransformToFloat(physics.boundary[1], 'X');
        float thisBottom = applyTransformToFloat(physics.boundary[2], 'Y');
        float thisTop = applyTransformToFloat(physics.boundary[3], 'Y');
        float thisFar = applyTransformToFloat(physics.boundary[4], 'Z');
        float thisNear = applyTransformToFloat(physics.boundary[5], 'Z');
    
        float otherLeft = objPtr -> applyTransformToFloat(objBoundary[0], 'X');
        float otherRight = objPtr -> applyTransformToFloat(objBoundary[1], 'X');
        float otherBottom = objPtr -> applyTransformToFloat(objBoundary[2], 'Y');
        float otherTop = objPtr -> applyTransformToFloat(objBoundary[3], 'Y');
        float otherFar = objPtr -> applyTransformToFloat(objBoundary[4], 'Z');
        float otherNear = objPtr -> applyTransformToFloat(objBoundary[5], 'Z');
    
        // AABB overlap test
        bool collisionOccured = false;

        if(thisFar == thisNear && otherNear == thisNear && otherFar == thisNear)
            collisionOccured = 
                (thisLeft < otherRight && thisRight > otherLeft) &&
                (thisBottom < otherTop && thisTop > otherBottom);

        else
            collisionOccured = 
                (thisLeft < otherRight && thisRight > otherLeft) &&
                (thisBottom < otherTop && thisTop > otherBottom) &&
                (thisFar < otherNear && thisNear > otherFar);

    
        if (!collisionOccured)
        {
            // std::cout <<"No Collision\n";
            return {false, 0.0f, glm::vec3(0.0f)};
        }
    
        std::vector<float> minimums = {
            otherRight - thisLeft,   // +X
            thisRight - otherLeft,   // -X
            otherTop - thisBottom,   // +Y
            thisTop - otherBottom  // -Y
            // otherNear - thisFar,     // +Z
            // thisNear - otherFar      // -Z
        };
    
        float minVal = std::numeric_limits<float>::max();
        int minIndex = -1;
        for (int i = 0, s = minimums.size(); i < s; ++i) {
            if (minimums[i] < 0) continue;
            if (minimums[i] < minVal) {
                minVal = minimums[i];
                minIndex = i;
            }
        }
        glm::vec3 normal(0.0f);
        // Calculate collision normal based on minimum overlap axis
        switch (minIndex) {
            case 0: normal =  glm::vec3(-1, 0, 0); break; // push left
            case 1: normal =  glm::vec3(1, 0, 0); break;  // push right
            case 2: normal =  glm::vec3(0, -1, 0); break; // push down
            case 3: normal =  glm::vec3(0, 1, 0);  break; // push up
            case 4: normal =  glm::vec3(0, 0, -1); break;// push back
            case 5: normal =  glm::vec3(0, 0, 1); break;  // push front
            default: normal =  glm::vec3(0);
        }

        collided collision = {true, minVal, normal};
        return collision;
    }

    collided checkSphericalCollision(gameObject *objPtr)
    {
        collided collision = {false, 0.0f, glm::vec3(0.0f)};
    
        if (!this->getCollisionStatus() || !objPtr->getCollisionStatus())
        {
            return collision;
        }

        float r1 = physics.radius;
        float r2 = objPtr->physics.radius;
    
        // glm::vec3 c1 = glm::vec3(model * glm::vec4(physics.position, 1.0f));
        // glm::vec3 c2 = glm::vec3(objPtr->model * glm::vec4(objPtr->physics.position, 1.0f));

        glm::vec4 origin(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 c1 = glm::vec3(model * objTranslation * origin);
        glm::vec3 c2 = glm::vec3(objPtr->model * objPtr->objTranslation * origin);

        std::cout << "C1:" << c1.x << " " << c1.y << " " << c1.z << " " << "C2:" << c2.x << " " << c2.y << " " << c2.z << "\n";
    
        glm::vec3 axis = c2 - c1;
        float dist = glm::length(axis);
        std::cout << "length :" << dist <<  "Sum of radii: " << r1+r2<< "\n";
    
        if (dist > r1 + r2)
            return collision; // no collision
    
        // Handle zero distance (perfect overlap) to avoid divide-by-zero
        if (dist != 0.0f)
            axis = glm::normalize(axis);
        else
            axis = glm::vec3(1.0f, 0.0f, 0.0f); // arbitrary axis if centers are exactly the same
    
        float penetrationDepth = r1 + r2 - dist;
        collision = {true, penetrationDepth, axis};

        std::cout<<"Collision\n";
        return collision;
    }

    collided checkSphereAABBCollision(gameObject *objPtr)
    {
        //get center of the objects
        // glm::vec3 c1 = glm::vec3(model * glm::vec4(physics.position, 1.0f));
        // glm::vec3 c2 = glm::vec3(objPtr->model * glm::vec4(objPtr->physics.position, 1.0f));

        if (!this->getCollisionStatus() || !objPtr->getCollisionStatus())
        {
            return {false, 0.0f, glm::vec3(0.0f)};
        }

        glm::vec4 origin(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 c1 = glm::vec3(model * objTranslation * origin);
        glm::vec3 c2 = glm::vec3(objPtr->model * objPtr->objTranslation * origin);

        if(this->isCircle)
        {
            std::vector<float> objBoundary = objPtr->getBoundary();

            float otherLeft = objPtr -> applyTransformToFloat(objBoundary[0], 'X');
            float otherRight = objPtr -> applyTransformToFloat(objBoundary[1], 'X');
            float otherBottom = objPtr -> applyTransformToFloat(objBoundary[2], 'Y');
            float otherTop = objPtr -> applyTransformToFloat(objBoundary[3], 'Y');
            float otherFar = objPtr -> applyTransformToFloat(objBoundary[4], 'Z');
            float otherNear = objPtr -> applyTransformToFloat(objBoundary[5], 'Z');

            float thisRadius = this->physics.radius;
            
            float closestX = std::max(otherLeft, std::min(c1.x, otherRight));
            float closestY = std::max(otherBottom, std::min(c1.y, otherTop));
            // float closestZ = std::max(otherFar, std::min(c1.z, otherNear));
            float closestZ = c1.z;

            glm::vec3 normal = glm::vec3(closestX, closestY, closestZ) - c1;

            if(glm::length(normal) > thisRadius)
                return {false, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f)};

            std::cout<<"Collision\n";
            return {true, thisRadius - glm::length(normal), glm::normalize(normal)};
        }
        else
        {
            float thisLeft = applyTransformToFloat(physics.boundary[0], 'X');
            float thisRight = applyTransformToFloat(physics.boundary[1], 'X');
            float thisBottom = applyTransformToFloat(physics.boundary[2], 'Y');
            float thisTop = applyTransformToFloat(physics.boundary[3], 'Y');
            float thisFar = applyTransformToFloat(physics.boundary[4], 'Z');
            float thisNear = applyTransformToFloat(physics.boundary[5], 'Z');

            float objRadius = objPtr->physics.radius;
            
            float closestX = std::max(thisLeft, std::min(c2.x, thisRight));
            float closestY = std::max(thisBottom, std::min(c2.y, thisTop));
            // float closestZ = std::max(thisFar, std::min(c2.z, thisNear));
            float closestZ = c2.z;

            glm::vec3 normal = c2 - glm::vec3(closestX, closestY, closestZ);

            if(glm::length(normal) > objRadius)
                return {false, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f)};

            std::cout<<"Collision\n";
            return {true, objRadius - glm::length(normal), glm::normalize(normal)};
        }
    }

    void overlapCorrection(collided collision, gameObject *objPtr)
    {
        float m1 = this->physics.mass;
        float m2 = objPtr->getMass();

        float penetrationDepth = collision.overlap;
        float percent = 0.8;
        float constant = 0;
        glm::vec3 correction;

        if(this->physics.isStatic){
            constant = penetrationDepth * percent;
            correction = constant * collision.normal;
            objPtr->physics.position -= correction;
        } 
        else if(objPtr -> physics.isStatic){
            constant = penetrationDepth * percent;
            correction = constant * collision.normal;
            this->physics.position -= correction;
        }
        else
        {
            constant = (penetrationDepth/(m1+m2)) * percent;
            correction = constant * collision.normal;
            this->physics.position += correction *m2;
            objPtr->physics.position -= correction *m1;
        }
    }
    
    void collision(gameObject *objPtr)
    {
        collided coll = {false, 0, glm::vec3(0.0f, 0.0f, 0.0f)};

        if(this->isCircle && objPtr->isCircle)
            coll = this->checkSphericalCollision(objPtr);
        else if(!this->isCircle && !objPtr->isCircle)
            coll = this->checkAABBCollision(objPtr);
        else
            coll = this->checkSphereAABBCollision(objPtr);
        
        // coll = this->checkSphericalCollision(objPtr);

        if(coll.hasCollided == false) return;

        //positional correction
        overlapCorrection(coll, objPtr);

        std::cout << "Collision\n" << coll.normal.x << " " << coll.normal.y << " " << coll.normal.z << std::endl;

        float e = (physics.coeffOfRestitution < objPtr->physics.coeffOfRestitution)? physics.coeffOfRestitution : objPtr->physics.coeffOfRestitution;

        glm::vec3 u1 = this->getVelocity();
        glm::vec3 u2 = objPtr->getVelocity();

        float j;
        glm::vec3 impulse, v1, v2;

        float m1 = this->getMass();
        float m2 = objPtr->getMass();

        glm::vec3 vRel = u1 - u2;

        float vAlongNormal = glm::dot(vRel, coll.normal);

        if(physics.isStatic)
        {
            j = -(1 + e) * vAlongNormal * m2;
            impulse = j * coll.normal;
            v1 = u1;
            v2 = u2 - impulse / m2;

            if(abs(v2.y) < 0.1) objPtr->physics.onGround = true;
        }
        else if(objPtr->physics.isStatic)
        {
            j = -(1 + e) * vAlongNormal * m1;
            impulse = j * coll.normal;
            v1 = u1 + impulse / m1;
            v2 = u2;

            if(abs(v1.y) < 0.1) physics.onGround = true;
        }
        else
        {
            j = -(1 + e) * vAlongNormal / (1/m1 + 1/m2);
            impulse = j * coll.normal;
            v1 = u1 + impulse / m1;
            v2 = u2 - impulse / m2;
        }

        this->setVelocity(v1);
        objPtr->setVelocity(v2);
    }

    void updatePhysics(float deltaTime)
    {
        if(physics.hasGravity && !physics.onGround)
        {
            physics.velocity += physics.acceleration * deltaTime;
        }

        if(physics.onGround)
        {
            physics.velocity.y = 0.0f;
        }

        physics.position += physics.velocity * deltaTime; 
        objTranslation = glm::translate(glm::mat4(1.0f), physics.position);
    }

    void fall()
    {
        glm::vec4 temp(physics.position, 1.0f);

        temp = view * model * temp;

        if(temp.x < -100)
            physics.hasGravity = true; 
    }

    //set functions to set vlaues in the class
    void setShader(shader* modelShader)
    {
        object.setShader(modelShader);
    }

    void setMass(float mass)
    {
        physics.mass = mass;
    }
    
    void setPosition(glm::vec3 position)
    {
        physics.position = position;
    }

    void setVelocity(glm::vec3 velocity)
    {
        physics.velocity = velocity;
    }

    void setAcceleration(glm::vec3 acceleration)
    {
        physics.acceleration += acceleration;
    }

    void setRestitution(float e)
    {
        physics.coeffOfRestitution = e;
    }

    void setStatic(bool isStatic)
    {
        physics.isStatic = isStatic;
    }

    void setGravityStatus(bool status)
    {
        physics.hasGravity = status;
    }

    void setModelMatrix(glm::mat4 matrix)
    {
        model = matrix;
    }

    bool setOnGroundStatus(bool status)
    {
        return physics.onGround = status;
    }

    //get function to get diff values from the class
    bool getGravityStatus()
    {
        return physics.hasGravity;
    }

    void setCollisionStatus(bool status)
    {
        physics.hasCollision = status;
    }

    bool getCollisionStatus()
    {
        return physics.hasCollision;
    }

    std::vector<float> getBoundary()
    {
        return physics.boundary;
    }

    float getMass()
    {
        return physics.mass;
    }

    glm::vec3 getPosition()
    {
        return physics.position;
    }

    glm::vec3 getVelocity()
    {
        return physics.velocity;
    }
 
    bool getOnGroundStatus()
    {
        return physics.onGround;
    }

    glm::mat4 getTransformationMatrix()
    {
        return objTranslation;
    }

    float getAverageVertices()
    {
        return object.getAverageVertices();
    }
    //rendering funtions
    void draw()
    {
        objTranslation = glm::translate(glm::mat4(1.0f), physics.position);
        object.useShader(model * objTranslation);
        object.draw(isCircle);
    }

};

class player : public gameObject {
    private:
    camera playerCam;
    //light?

    public:
    player(shader* modelShader)
    {
        object.setShader(modelShader);
        object.setColor(glm::vec4(0.4f,0.4f,0.8f, 1.0f));

        physics.mass = 100.0f;
        physics.hasGravity = true;
        physics.acceleration = GRAVITY;

        physics.onGround = false;
        physics.hasCollision = true;
        physics.coeffOfRestitution = 0.0f;
        // physics.boundary = object.getBoundary();
        
        glm::vec4 temp(physics.position, 1.0f);
        temp = temp * model;

        playerCam.setCamera(glm::vec3(temp.x, 0.0f, 50.0f), glm::vec3(temp.x, 0.0f, temp.z), glm::vec3(0.0f, 1.0f, 0.0f));

    }

    player(shader* modelShader, float length, float breadth, float width = 0)
    {
        object.setShader(modelShader);
        object.setColor(glm::vec4(0.4f,0.4f,0.8f, 1.0f));

        physics.mass = 100.0f;
        physics.hasGravity = true;
        physics.acceleration = GRAVITY;

        physics.onGround = false;
        physics.hasCollision = true;
        physics.coeffOfRestitution = 0.0f;
        // physics.boundary = object.getBoundary();

        if(width == 0)
            block2D(length, breadth);

        
        glm::vec4 temp(physics.position, 1.0f);
        temp = temp * model;

        playerCam.setCamera(glm::vec3(temp.x, 0.0f, 50.0f), glm::vec3(temp.x, 0.0f, temp.z), glm::vec3(0.0f, 1.0f, 0.0f));

    }

    player(shader* modelShader, float radius)
    {
        object.setShader(modelShader);
        object.setColor(glm::vec4(0.4f,0.4f,0.8f, 1.0f));

        physics.mass = 100.0f;
        physics.hasGravity = true;
        physics.acceleration = GRAVITY;

        physics.onGround = false;
        physics.hasCollision = true;
        physics.coeffOfRestitution = 0.0f;
        // physics.boundary = object.getBoundary();

        circle2D(radius);
        
        glm::vec4 temp(physics.position, 1.0f);
        temp = temp * model;

        playerCam.setCamera(glm::vec3(temp.x, 0.0f, 50.0f), glm::vec3(temp.x, 0.0f, temp.z), glm::vec3(0.0f, 1.0f, 0.0f));

    }

    void jump()
    {
        if(physics.onGround == true)
        {            
            physics.velocity.y = 85.0f;
            // physics.acceleration.y = GRAVITY.y - 60.0f;
            physics.onGround = false;
        }
        // else physics.acceleration.y = GRAVITY.y;
    }

    //camera stuff
    void updateCamera()
    {

        glm::vec4 temp(physics.position, 1.0f);
        temp = temp * model;

        // playerCam.setCamera(glm::vec3(temp.x, temp.y, 50.0f), glm::vec3(temp.x, temp.y, temp.z), glm::vec3(0.0f, 1.0f, 0.0f));


        playerCam.setCamera(glm::vec3(temp.x, 0.0f, 50.0f), glm::vec3(temp.x, 0.0f, temp.z), glm::vec3(0.0f, 1.0f, 0.0f));

        // playerCam.updateCameraPosition(model * objTranslation);
        // playerCam.updateTargetPosition(model * objTranslation);
    }
    
    //get functions
    glm::mat4 getViewMatrix()
    {
        return playerCam.viewMatrix();
    }

    //set functions
    void setCamera(glm::vec3 camPos, glm::vec3 targetPos, glm::vec3 up){
        playerCam.setCamera(camPos, targetPos, up);
    }

    void updateCameraPosition(glm::mat4 matrix)
    {
        playerCam.updateCameraPosition(matrix);
    }

    void updateCameraPosition(glm::vec3 position)
    {
        playerCam.updateCameraPosition(position);
    }

    void updateTargetPosition(glm::vec3 target)
    {
        playerCam.updateTargetPosition(target);
    }

    void updateTargetPosition(glm::mat4 matrix)
    {
        playerCam.updateTargetPosition(matrix);
    }

};

class obstacle : public gameObject {
    public:
    obstacle()
    {
        object.setColor(glm::vec4(0.8f,0.8f,0.8f, 1.0f));
        // physics.hasGravity = true;
        // physics.acceleration = GRAVITY;

        physics.hasCollision = true;
        physics.isStatic = true;
    }

    obstacle(shader* modelShader, float length, float breadth, float width = 0)
    {
        object.setShader(modelShader);
        object.setColor(glm::vec4(0.8f,0.8f,0.8f, 1.0f));

        physics.hasGravity = true;
        physics.acceleration = GRAVITY;

        physics.hasCollision = true;
        // physics.isStatic = true;

        if(width == 0)
            block2D(length, breadth);
    }
};

class ground : public gameObject {
    public:
    ground(shader* modelShader, float length, float breadth, float width = 0)
    {
        object.setShader(modelShader);
        object.setColor(glm::vec4(0.2f,0.8f,0.2f, 1.0f));

        physics.mass = 500;
        physics.acceleration = GRAVITY;

        physics.hasCollision = true;
        physics.isStatic = true;

        if(width == 0)
            block2D(length, breadth);
    }

    ground(shader* modelShader, float radius)
    {
        object.setShader(modelShader);
        object.setColor(glm::vec4(0.2f,0.8f,0.2f, 1.0f));

        physics.mass = 500;
        physics.acceleration = GRAVITY;

        physics.hasCollision = true;
        physics.isStatic = true;
        // physics.boundary = object.getBoundary();

        circle2D(radius);
    }
};