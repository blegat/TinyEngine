struct Branch{

  int ID = 0;         //For Leaf Hashing
  bool leaf = true;
  Branch* A;
  Branch* B;

  //Parameters
  float ratio, spread, splitsize;
  int depth = 0;

  Branch(float r, float s, float ss){
    ratio = r;
    spread = s;
    splitsize = ss;
  };

  ~Branch(){
    if(leaf) return;
    delete(A);
    delete(B);
  }

  //Position Data
  glm::vec3 dir = glm::vec3(0.0, 1.0, 0.0);
  double length = 0.0;
  double radius = 0.0;
  double area = 0.1;

  void grow(double _size);
  void split();
};

class Tree{
public:
  Tree(){ root = new Branch(0.6, 0.6, 2.5); }
  ~Tree(){ delete root; }

  float rate = 1.0;

  Branch* root;
  void grow(){
    root->grow(rate);
  }
};

void Branch::grow(double feed){

  //Compute Current Radius
  radius = sqrt(area/PI);

  if(leaf){
    length += cbrt(feed);  //Grow in Length
    feed -= length * area; //Reduce Feed
    area += feed/length;   //Grow In Area

    //Split Condition
    if(length > splitsize * exp(-splitdecay * depth))
      split();  //Split Behavior

    return;
  }

  //We are not a leaf!

  double pass = passratio;

  if(conservearea)
    pass = (A->area+B->area)/(A->area+B->area+area);
  else if(conservediameter)
    pass = (A->radius+B->radius)/(A->radius+B->radius+radius);

  area += pass * feed / length; //Grow in Girth
  feed *= ( 1.0 - pass );

  //Just to Prevent too large depth
  if(feed < 1E-6) return;

  A->grow(ratio*feed);
  B->grow((1-ratio)*feed);
}

void Branch::split(){
  leaf = false;

  A = new Branch(ratio, spread, splitsize);
  B = new Branch(ratio, spread, splitsize);
  A->ID = rand()%1000;
  B->ID = rand()%1000;
  A->depth = depth+1;
  B->depth = depth+1;

  //Random Vector in Space
  glm::vec3 O = glm::normalize(glm::vec3((double)(rand()%100)/100.0-0.5, (double)(rand()%100)/100.0, (double)(rand()%100)/100.0-0.5));

  //Randomly Oriented Normal Vector to Previous Branch
  glm::vec3 N = glm::cross(glm::normalize(dir), O);

  //Reflect around vertical axis
  glm::vec3 M = glm::vec3(-1.0, 1.0, -1.0)*N;

  //Set Directions
  A->dir = glm::normalize( glm::mix(spread*N, dir, ratio) );
  B->dir = glm::normalize( glm::mix(spread*M, dir, 1.0-ratio) );
}

Tree tree;


// Model Constructing Function for Tree
std::function<void(Model*)> _construct = [&](Model* h){

  //Basically Add Lines for the Tree!
  std::function<void(Branch*, glm::vec3)> addBranch = [&](Branch* b, glm::vec3 p){

    glm::vec3 start = p;
    glm::vec3 end   = p + glm::vec3(b->length*treescale[0])*b->dir;

    //Get Some Normal Vector
    glm::vec3 x = glm::normalize(b->dir + glm::vec3(1.0, 1.0, 1.0));
    glm::vec4 n = glm::vec4(glm::normalize(glm::cross(b->dir, x)), 1.0);

    //Add the Correct Number of Indices
    glm::mat4 r = glm::rotate(glm::mat4(1.0), PI/ringsize, b->dir);

    //Index Buffer
    int _b = h->positions.size()/3;

    //GL TRIANGLES
    for(int i = 0; i < ringsize; i++){
      //Bottom Triangle
      h->indices.push_back(_b+i*2+0);
      h->indices.push_back(_b+i*2+1);
      h->indices.push_back(_b+(i*2+2)%(2*ringsize));
      //Upper Triangle
      h->indices.push_back(_b+(i*2+2)%(2*ringsize));
      h->indices.push_back(_b+i*2+1);
      h->indices.push_back(_b+(i*2+3)%(2*ringsize));
    }

    for(int i = 0; i < ringsize; i++){

      h->positions.push_back(start.x + b->radius*treescale[1]*n.x);
      h->positions.push_back(start.y + b->radius*treescale[1]*n.y);
      h->positions.push_back(start.z + b->radius*treescale[1]*n.z);
      h->normals.push_back(n.x);
      h->normals.push_back(n.y);
      h->normals.push_back(n.z);
      n = r*n;

      h->positions.push_back(end.x + taper*b->radius*treescale[1]*n.x);
      h->positions.push_back(end.y + taper*b->radius*treescale[1]*n.y);
      h->positions.push_back(end.z + taper*b->radius*treescale[1]*n.z);
      h->normals.push_back(n.x);
      h->normals.push_back(n.y);
      h->normals.push_back(n.z);
      n = r*n;

    }

    //No Leaves
    if(b->leaf) return;

    addBranch(b->A, end);
    addBranch(b->B, end);
  };

  //Recursive add Branches
  addBranch(tree.root, glm::vec3(0.0));
};

//Hash Function for Leaf Displacement!
std::hash<std::string> position_hash;
double hashrand(int i){
  return (double)(position_hash(std::to_string(i))%1000)/1000.0;
}

//Construct Leaf Particle System from Tree Data
std::function<void(Particle*)> addLeaves = [&](Particle* p){
  p->models.clear();

  //Explore the Tree and Add Leaves!
  std::function<void(Branch*, glm::vec3)> addLeaf = [&](Branch* b, glm::vec3 pos){

    if(b->leaf){

      if(b->depth < leafmindepth) return;

      for(int i = 0; i < leafcount; i++){
        //Hashed Random Displace
        glm::vec3 d = glm::vec3(hashrand(b->ID+i), hashrand(b->ID+i+leafcount), hashrand(b->ID+i+2*leafcount))-glm::vec3(0.5);
        d = d * glm::vec3(leafspread[0], leafspread[1], leafspread[2]);

        //Rotate Towards Camera and Scale
        glm::mat4 model = glm::translate(glm::mat4(1.0), pos+d);
        model = glm::rotate(model, -glm::radians(rotation - 45.0f), glm::vec3(0.0, 1.0, 0.0));
        p->models.push_back(glm::scale(model, glm::vec3(leafsize)));

      }

      return;
    }

    //Otherwise get other leaves!
    glm::vec3 end   = pos + glm::vec3(b->length*treescale[0])*b->dir;
    addLeaf(b->A, end);
    addLeaf(b->B, end);
  };

  addLeaf(tree.root, glm::vec3(0.0));
};
