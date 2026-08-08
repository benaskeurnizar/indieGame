

void renderQuakeFace(Engine* engine,face_render* face,unsigned int tex_loc){
  int face_index = face->face_index;
  int texinfo_id = engine->world.tree->faces[face_index].texinfo_id;
  texinfo_t* texinfo = &engine->world.tree->texinfo[texinfo_id];
  int miptex_id = texinfo->texture_id;
  unsigned int tex_id = engine->world.tree->my_texes[miptex_id].tex_id;

  unsigned int shader_program = engine->renderer.MapShader.ProgramShader;
  int brightness_loc = glGetUniformLocation(shader_program, "brightness");
  // in your render loop, before drawing each face:
  glUniform1f(brightness_loc, face->brightness);
  // Bind texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glUniform1i(tex_loc, 0);
        
  // Bind VAO and draw
  glBindVertexArray(face->VAO);
  glDrawArrays(GL_TRIANGLES, 0, face->num_vertices);
}

void renderMeshedQuakeFace(Engine* engine,MeshedFace* face,unsigned int tex_loc){
  int face_index = face->face_id;
  int texinfo_id = engine->world.tree->faces[face_index].texinfo_id;
  texinfo_t* texinfo = &engine->world.tree->texinfo[texinfo_id];
  int miptex_id = texinfo->texture_id;
  unsigned int tex_id = engine->world.tree->my_texes[miptex_id].tex_id;

  unsigned int shader_program = engine->renderer.MapShader.ProgramShader;
  // in your render loop, before drawing each face:
  //glUniform1f(brightness_loc, face->brightness);
  // Bind texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_id);
  glUniform1i(tex_loc, 0);
        
  // Bind VAO and draw
  glBindVertexArray(face->VAO);
  glDrawArrays(GL_TRIANGLES, 0, face->num_vertices);
}

void renderFrameFaces(Engine* engine){
  glViewport(0,0,engine->renderer.screenWidth,engine->renderer.screenHeight);
  
  glClearColor(0.6,0.6,0.8,1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  unsigned int shader_program = engine->renderer.MapShader.ProgramShader;
  glUseProgram(shader_program);
    
    // Set matrices
  unsigned int view_loc = glGetUniformLocation(shader_program, "view");
  unsigned int proj_loc = glGetUniformLocation(shader_program, "projection");
  unsigned int model_loc = glGetUniformLocation(shader_program, "model");

  mat4 model;
  glm_mat4_identity(model);
  if(engine->debug_mode){
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)engine->world.debugCamera.view);
  }else{
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float*)engine->world.camera.view);
  }
  glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float*)engine->renderer.proj);
  glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float*)model);
    
  glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);
  unsigned int tex_loc = glGetUniformLocation(shader_program, "texture0");
  for(int i=0;i<engine->world.num_faces;i++){
    //face_render* face = &engine->world.faces[i];
    // renderQuakeFace(engine,face,tex_loc);
    int face_index = engine->world.faces_indexes[i];
    MeshedFace face = engine->world.meshed_faces[face_index] ; 
    renderMeshedQuakeFace(engine,&face,tex_loc);
  }
}



// create big triangulated geometry at the start of the game,store each triangle with it's brightness values and textures and stuff,and on render time get
// those triangles from the parent face and render
