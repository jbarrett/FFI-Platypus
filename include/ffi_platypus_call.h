    /* buffer contains the memory required for the arguments structure */
    buffer_size = sizeof(ffi_pl_argument) * self->ffi_cif.nargs * 2 + sizeof(ffi_pl_arguments);
#ifdef HAVE_ALLOCA
    buffer = alloca(buffer_size);
#else
    Newx(buffer, buffer_size, char);
#endif
    current_argv = arguments = (ffi_pl_arguments*) buffer;

    arguments->count = self->ffi_cif.nargs;

    /*
     * ARGUMENT IN
     */

    for(i=0; i < self->ffi_cif.nargs; i++)
    {
      int platypus_type = self->argument_types[i]->platypus_type;
      ((void**)&arguments->slot[arguments->count])[i] = &arguments->slot[i];

      arg = i+(EXTRA_ARGS) < items ? ST(i+(EXTRA_ARGS)) : &PL_sv_undef;
      if(platypus_type == FFI_PL_NATIVE)
      {
        switch(self->argument_types[i]->ffi_type->type)
        {
          case FFI_TYPE_UINT8:
            ffi_pl_arguments_set_uint8(arguments, i, SvOK(arg) ? SvUV(arg) : 0);
            break;
          case FFI_TYPE_SINT8:
            ffi_pl_arguments_set_sint8(arguments, i, SvOK(arg) ? SvIV(arg) : 0);
            break;
          case FFI_TYPE_UINT16:
            ffi_pl_arguments_set_uint16(arguments, i, SvOK(arg) ? SvUV(arg) : 0);
            break;
          case FFI_TYPE_SINT16:
            ffi_pl_arguments_set_sint16(arguments, i, SvOK(arg) ? SvIV(arg) : 0);
            break;
          case FFI_TYPE_UINT32:
            ffi_pl_arguments_set_uint32(arguments, i, SvOK(arg) ? SvUV(arg) : 0);
            break;
          case FFI_TYPE_SINT32:
            ffi_pl_arguments_set_sint32(arguments, i, SvOK(arg) ? SvIV(arg) : 0);
            break;
#ifdef HAVE_IV_IS_64
          case FFI_TYPE_UINT64:
            ffi_pl_arguments_set_uint64(arguments, i, SvOK(arg) ? SvUV(arg) : 0);
            break;
          case FFI_TYPE_SINT64:
            ffi_pl_arguments_set_sint64(arguments, i, SvOK(arg) ? SvIV(arg) : 0);
            break;
#else
          case FFI_TYPE_UINT64:
            ffi_pl_arguments_set_uint64(arguments, i, SvOK(arg) ? SvU64(arg) : 0);
            break;
          case FFI_TYPE_SINT64:
            ffi_pl_arguments_set_sint64(arguments, i, SvOK(arg) ? SvI64(arg) : 0);
            break;
#endif
          case FFI_TYPE_FLOAT:
            ffi_pl_arguments_set_float(arguments, i, SvOK(arg) ? SvNV(arg) : 0.0);
            break;
          case FFI_TYPE_DOUBLE:
            ffi_pl_arguments_set_double(arguments, i, SvOK(arg) ? SvNV(arg) : 0.0);
            break;
          case FFI_TYPE_POINTER:
            ffi_pl_arguments_set_pointer(arguments, i, SvOK(arg) ? INT2PTR(void*, SvIV(arg)) : NULL);
            break;
          default:
            warn("argument type not supported (%d)", i);
            break;
        }
      }
      else if(platypus_type == FFI_PL_STRING)
      {
        switch(self->argument_types[i]->extra[0].string.platypus_string_type)
        {
          case FFI_PL_STRING_RW:
          case FFI_PL_STRING_RO:
            ffi_pl_arguments_set_string(arguments, i, SvOK(arg) ? SvPV_nolen(arg) : NULL);
            break;
          case FFI_PL_STRING_FIXED:
            {
              int expected;
              STRLEN size;
              void *ptr;
              expected = self->argument_types[i]->extra[0].string.size;
              ptr = SvOK(arg) ? SvPV(arg, size) : NULL;
              if(ptr != NULL && expected != 0 && size != expected)
                warn("fixed string argument %d has wrong size (is %d, expected %d)", i, (int)size, expected);
              ffi_pl_arguments_set_pointer(arguments, i, ptr);
            }
            break;
        }
      }
      else if(platypus_type == FFI_PL_POINTER)
      {
        void *ptr;
        if(SvROK(arg)) /* TODO: and a scalar ref */
        {
          SV *arg2 = SvRV(arg);
          switch(self->argument_types[i]->ffi_type->type)
          {
            case FFI_TYPE_UINT8:
              Newx_or_alloca(ptr, uint8_t);
              *((uint8_t*)ptr) = SvOK(arg2) ? SvUV(arg2) : 0;
              break;
            case FFI_TYPE_SINT8:
              Newx_or_alloca(ptr, int8_t);
              *((int8_t*)ptr) = SvOK(arg2) ? SvIV(arg2) : 0;
              break;
            case FFI_TYPE_UINT16:
              Newx_or_alloca(ptr, uint16_t);
              *((uint16_t*)ptr) = SvOK(arg2) ? SvUV(arg2) : 0;
              break;
            case FFI_TYPE_SINT16:
              Newx_or_alloca(ptr, int16_t);
              *((int16_t*)ptr) = SvOK(arg2) ? SvIV(arg2) : 0;
              break;
            case FFI_TYPE_UINT32:
              Newx_or_alloca(ptr, uint32_t);
              *((uint32_t*)ptr) = SvOK(arg2) ? SvUV(arg2) : 0;
              break;
            case FFI_TYPE_SINT32:
              Newx_or_alloca(ptr, int32_t);
              *((int32_t*)ptr) = SvOK(arg2) ? SvIV(arg2) : 0;
              break;
            case FFI_TYPE_UINT64:
              Newx_or_alloca(ptr, uint64_t);
#ifdef HAVE_IV_IS_64
              *((uint64_t*)ptr) = SvOK(arg2) ? SvUV(arg2) : 0;
#else
              *((uint64_t*)ptr) = SvOK(arg2) ? SvU64(arg2) : 0;
#endif
              break;
            case FFI_TYPE_SINT64:
              Newx_or_alloca(ptr, int64_t);
#ifdef HAVE_IV_IS_64
              *((int64_t*)ptr) = SvOK(arg2) ? SvIV(arg2) : 0;
#else
              *((int64_t*)ptr) = SvOK(arg2) ? SvI64(arg2) : 0;
#endif
              break;
            case FFI_TYPE_FLOAT:
              Newx_or_alloca(ptr, float);
              *((float*)ptr) = SvOK(arg2) ? SvNV(arg2) : 0.0;
              break;
            case FFI_TYPE_DOUBLE:
              Newx_or_alloca(ptr, double);
              *((double*)ptr) = SvOK(arg2) ? SvNV(arg2) : 0.0;
              break;
            case FFI_TYPE_POINTER:
              Newx_or_alloca(ptr, void*);
              {
                SV *tmp = SvRV(arg);
                *((void**)ptr) = SvOK(tmp) ? INT2PTR(void *, SvIV(tmp)) : NULL;
              }
              break;
            default:
              warn("argument type not supported (%d)", i);
              *((void**)ptr) = NULL;
              break;
          }
        }
        else
        {
          ptr = NULL;
        }
        ffi_pl_arguments_set_pointer(arguments, i, ptr);
      }
      else if(platypus_type == FFI_PL_RECORD)
      {
        void *ptr;
        STRLEN size;
        int expected;
        expected = self->argument_types[i]->extra[0].record.size;
        if(SvROK(arg))
        {
          SV *arg2 = SvRV(arg);
          ptr = SvOK(arg2) ? SvPV(arg2, size) : NULL;
        }
        else
        {
          ptr = SvOK(arg) ? SvPV(arg, size) : NULL;
        }
        if(ptr != NULL && expected != 0 && size != expected)
          warn("record argument %d has wrong size (is %d, expected %d)", i, (int)size, expected);
        ffi_pl_arguments_set_pointer(arguments, i, ptr);
      }
      else if(platypus_type == FFI_PL_ARRAY)
      {
        void *ptr;
        int count = self->argument_types[i]->extra[0].array.element_count;
        if(SvROK(arg) && SvTYPE(SvRV(arg)) == SVt_PVAV)
        {
          AV *av = (AV*) SvRV(arg);
          if(count == 0)
            count = av_len(av)+1;
          switch(self->argument_types[i]->ffi_type->type)
          {
            case FFI_TYPE_UINT8:
              Newx(ptr, count, uint8_t);
              for(n=0; n<count; n++)
              {
                ((uint8_t*)ptr)[n] = SvUV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_SINT8:
              Newx(ptr, count, int8_t);
              for(n=0; n<count; n++)
              {
                ((int8_t*)ptr)[n] = SvIV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_UINT16:
              Newx(ptr, count, uint16_t);
              for(n=0; n<count; n++)
              {
                ((uint16_t*)ptr)[n] = SvUV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_SINT16:
              Newx(ptr, count, int16_t);
              for(n=0; n<count; n++)
              {
                ((int16_t*)ptr)[n] = SvIV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_UINT32:
              Newx(ptr, count, uint32_t);
              for(n=0; n<count; n++)
              {
                ((uint32_t*)ptr)[n] = SvUV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_SINT32:
              Newx(ptr, count, int32_t);
              for(n=0; n<count; n++)
              {
                ((int32_t*)ptr)[n] = SvIV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_UINT64:
              Newx(ptr, count, uint64_t);
              for(n=0; n<count; n++)
              {
#ifdef HAVE_IV_IS_64
                ((uint64_t*)ptr)[n] = SvUV(*av_fetch(av, n, 1));
#else
                ((uint64_t*)ptr)[n] = SvU64(*av_fetch(av, n, 1));
#endif
              }
              break;
            case FFI_TYPE_SINT64:
              Newx(ptr, count, int64_t);
              for(n=0; n<count; n++)
              {
#ifdef HAVE_IV_IS_64
                ((int64_t*)ptr)[n] = SvIV(*av_fetch(av, n, 1));
#else
                ((int64_t*)ptr)[n] = SvI64(*av_fetch(av, n, 1));
#endif
              }
              break;
            case FFI_TYPE_FLOAT:
              Newx(ptr, count, float);
              for(n=0; n<count; n++)
              {
                ((float*)ptr)[n] = SvNV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_DOUBLE:
              Newx(ptr, count, double);
              for(n=0; n<count; n++)
              {
                ((double*)ptr)[n] = SvNV(*av_fetch(av, n, 1));
              }
              break;
            case FFI_TYPE_POINTER:
              Newx(ptr, count, void*);
              for(n=0; n<count; n++)
              {
                SV *sv = *av_fetch(av, n, 1);
                ((void**)ptr)[n] = SvOK(sv) ? INT2PTR(void*, SvIV(sv)) : NULL;
              }
              break;
            default:
              Newxz(ptr, count*self->argument_types[i]->ffi_type->size, char);
              warn("argument type not supported (%d)", i);
              break;
          }
        }
        else
        {
          warn("passing non array reference into ffi/platypus array argument type");
          Newxz(ptr, count*self->argument_types[i]->ffi_type->size, char);
        }
        ffi_pl_arguments_set_pointer(arguments, i, ptr);
      }
      else if(platypus_type == FFI_PL_CLOSURE)
      {
        if(!SvROK(arg))
        {
          ffi_pl_arguments_set_pointer(arguments, i, NULL);
        }
        else
        {
          ffi_pl_closure *closure;
          ffi_status ffi_status;

          SvREFCNT_inc(arg);

          Newx(closure, 1, ffi_pl_closure);
          closure->ffi_closure = ffi_closure_alloc(sizeof(ffi_closure), &closure->function_pointer);
          if(closure->ffi_closure == NULL)
          {
            Safefree(closure);
            ffi_pl_arguments_set_pointer(arguments, i, NULL);
            warn("unable to allocate memory for closure");
          }
          else
          {
            closure->type = self->argument_types[i];

            ffi_status = ffi_prep_closure_loc(
              closure->ffi_closure,
              &self->argument_types[i]->extra[0].closure.ffi_cif,
              ffi_pl_closure_call,
              closure,
              closure->function_pointer
            );

            if(ffi_status != FFI_OK)
            {
              ffi_closure_free(closure->ffi_closure);
              Safefree(closure);
              ffi_pl_arguments_set_pointer(arguments, i, NULL);
              warn("unable to create closure");
            }
            else
            {
              closure->coderef = arg;
              ffi_pl_closure_add_data(arg, closure);
              ffi_pl_arguments_set_pointer(arguments, i, closure->function_pointer);
            }
          }
        }
      }
      else if(platypus_type == FFI_PL_CUSTOM_PERL)
      {
        SV *arg2 = ffi_pl_custom_perl(
          self->argument_types[i]->extra[0].custom_perl.perl_to_native,
          arg,
          i
        );

        if(arg2 != NULL)
        {
          switch(self->argument_types[i]->ffi_type->type)
          {
            case FFI_TYPE_UINT8:
              ffi_pl_arguments_set_uint8(arguments, i, SvUV(arg2));
              break;
            case FFI_TYPE_SINT8:
              ffi_pl_arguments_set_sint8(arguments, i, SvIV(arg2));
              break;
            case FFI_TYPE_UINT16:
              ffi_pl_arguments_set_uint16(arguments, i, SvUV(arg2));
              break;
            case FFI_TYPE_SINT16:
              ffi_pl_arguments_set_sint16(arguments, i, SvIV(arg2));
              break;
            case FFI_TYPE_UINT32:
              ffi_pl_arguments_set_uint32(arguments, i, SvUV(arg2));
              break;
            case FFI_TYPE_SINT32:
              ffi_pl_arguments_set_sint32(arguments, i, SvIV(arg2));
              break;
#ifdef HAVE_IV_IS_64
            case FFI_TYPE_UINT64:
              ffi_pl_arguments_set_uint64(arguments, i, SvUV(arg2));
              break;
            case FFI_TYPE_SINT64:
              ffi_pl_arguments_set_sint64(arguments, i, SvIV(arg2));
              break;
#else
            case FFI_TYPE_UINT64:
              ffi_pl_arguments_set_uint64(arguments, i, SvU64(arg2));
              break;
            case FFI_TYPE_SINT64:
              ffi_pl_arguments_set_sint64(arguments, i, SvI64(arg2));
              break;
#endif
            case FFI_TYPE_FLOAT:
              ffi_pl_arguments_set_float(arguments, i, SvNV(arg2));
              break;
            case FFI_TYPE_DOUBLE:
              ffi_pl_arguments_set_double(arguments, i, SvNV(arg2));
              break;
            case FFI_TYPE_POINTER:
              ffi_pl_arguments_set_pointer(arguments, i, SvOK(arg2) ? INT2PTR(void*, SvIV(arg2)) : NULL);
              break;
            default:
              warn("argument type not supported (%d)", i);
              break;
          }
          SvREFCNT_dec(arg2);
        }

        for(n=0; n < self->argument_types[i]->extra[0].custom_perl.argument_count; n++)
        {
          i++;
          ((void**)&arguments->slot[arguments->count])[i] = &arguments->slot[i];
        }
      }
      else if(platypus_type == FFI_PL_EXOTIC_FLOAT)
      {
        switch(self->argument_types[i]->ffi_type->type)
        {
#ifdef SIZEOF_LONG_DOUBLE
          case FFI_TYPE_LONGDOUBLE:
            {
              long double *ptr;
              Newx_or_alloca(ptr, long double);
              ((void**)&arguments->slot[arguments->count])[i] = (void*)ptr;
              if(sv_isobject(arg) && sv_derived_from(arg, "Math::LongDouble"))
              {
                *ptr = *INT2PTR(long double *, SvIV((SV*) SvRV(arg)));
              }
              else
              {
                *ptr = SvNV(arg);
              }
            }
            break;
#endif
#ifdef FFI_TARGET_HAS_COMPLEX_TYPE
          case FFI_TYPE_COMPLEX:
            switch(self->argument_types[i]->ffi_type->size)
            {
#if SIZEOF_FLOAT_COMPLEX
              case  8:
                {
                  float complex *ptr;
                  Newx_or_alloca(ptr, float complex);
                  ((void**)&arguments->slot[arguments->count])[i] = (void*)ptr;
                }
                break;
#endif
#if SIZEOF_DOUBLE_COMPLEX
              case 16:
                {
                  double complex *ptr;
                  Newx_or_alloca(ptr, double complex);
                  ((void**)&arguments->slot[arguments->count])[i] = (void*)ptr;
                }
                break;
#endif
              default :
                warn("argument type not supported (%d)", i);
                break;
            }
            break;
#endif
          default:
            warn("argument type not supported (%d)", i);
            break;
        }
      }
      else
      {
        warn("argument type not supported (%d)", i);
      }
    }

    /*
     * CALL
     */

#if 0
    fprintf(stderr, "# ===[%p]===\n", self->address);
    for(i=0; i < self->ffi_cif.nargs; i++)
    {
      fprintf(stderr, "# [%d] <%d:%d> %p %p %016llx \n",
        i,
        self->argument_types[i]->ffi_type->type,
        self->argument_types[i]->platypus_type,
        ((void**)&arguments->slot[arguments->count])[i],
        &arguments->slot[i],
        ffi_pl_arguments_get_uint64(arguments, i)
        /* ffi_pl_arguments_get_float(arguments, i), *
         * ffi_pl_arguments_get_double(arguments, i) */
      );
    }
    fprintf(stderr, "# === ===\n");
    fflush(stderr);
#endif

    current_argv = NULL;

    if(self->address != NULL)
    {
      ffi_call(&self->ffi_cif, self->address, &result, ffi_pl_arguments_pointers(arguments));
    }
    else
    {
      void *address = self->ffi_cif.nargs > 0 ? (void*) &cast1 : (void*) &cast0;
      ffi_call(&self->ffi_cif, address, &result, ffi_pl_arguments_pointers(arguments));
    }

    /*
     * ARGUMENT OUT
     */

    current_argv = arguments;

    for(i=self->ffi_cif.nargs-1; i >= 0; i--)
    {
      platypus_type platypus_type;
    
      if(platypus_type == FFI_PL_POINTER)
      {
        void *ptr = ffi_pl_arguments_get_pointer(arguments, i);
        if(ptr != NULL)
        {
          arg = i+(EXTRA_ARGS) < items ? ST(i+(EXTRA_ARGS)) : &PL_sv_undef;
          if(!SvREADONLY(SvRV(arg)))
          {
            switch(self->argument_types[i]->ffi_type->type)
            {
              case FFI_TYPE_UINT8:
                sv_setuv(SvRV(arg), *((uint8_t*)ptr));
                break;
              case FFI_TYPE_SINT8:
                sv_setiv(SvRV(arg), *((int8_t*)ptr));
                break;
              case FFI_TYPE_UINT16:
                sv_setuv(SvRV(arg), *((uint16_t*)ptr));
                break;
              case FFI_TYPE_SINT16:
                sv_setiv(SvRV(arg), *((int16_t*)ptr));
                break;
              case FFI_TYPE_UINT32:
                sv_setuv(SvRV(arg), *((uint32_t*)ptr));
                break;
              case FFI_TYPE_SINT32:
                sv_setiv(SvRV(arg), *((int32_t*)ptr));
                break;
              case FFI_TYPE_UINT64:
#ifdef HAVE_IV_IS_64
                sv_setuv(SvRV(arg), *((uint64_t*)ptr));
#else
                sv_setu64(SvRV(arg), *((uint64_t*)ptr));
#endif
                break;
              case FFI_TYPE_SINT64:
#ifdef HAVE_IV_IS_64
                sv_setiv(SvRV(arg), *((int64_t*)ptr));
#else
                sv_seti64(SvRV(arg), *((int64_t*)ptr));
#endif
                break;
              case FFI_TYPE_FLOAT:
                sv_setnv(SvRV(arg), *((float*)ptr));
                break;
              case FFI_TYPE_POINTER:
                if( *((void**)ptr) == NULL)
                  sv_setsv(SvRV(arg), &PL_sv_undef);
                else
                  sv_setiv(SvRV(arg), PTR2IV(*((void**)ptr)));
                break;
              case FFI_TYPE_DOUBLE:
                sv_setnv(SvRV(arg), *((double*)ptr));
                break;
            }
          }
        }
#ifndef HAVE_ALLOCA
        Safefree(ptr);
#endif
      }
      else if(platypus_type == FFI_PL_ARRAY)
      {
        void *ptr = ffi_pl_arguments_get_pointer(arguments, i);
        int count = self->argument_types[i]->extra[0].array.element_count;
        arg = i+(EXTRA_ARGS) < items ? ST(i+(EXTRA_ARGS)) : &PL_sv_undef;
        if(SvROK(arg) && SvTYPE(SvRV(arg)) == SVt_PVAV)
        {
          AV *av = (AV*) SvRV(arg);
          switch(self->argument_types[i]->ffi_type->type)
          {
            case FFI_TYPE_UINT8:
              for(n=0; n<count; n++)
              {
                sv_setuv(*av_fetch(av, n, 1), ((uint8_t*)ptr)[n]);
              }
              break;
            case FFI_TYPE_SINT8:
              for(n=0; n<count; n++)
              {
                sv_setiv(*av_fetch(av, n, 1), ((int8_t*)ptr)[n]);
              }
              break;
            case FFI_TYPE_UINT16:
              for(n=0; n<count; n++)
              {
                sv_setuv(*av_fetch(av, n, 1), ((uint16_t*)ptr)[n]);
              }
              break;
            case FFI_TYPE_SINT16:
              for(n=0; n<count; n++)
              {
                sv_setiv(*av_fetch(av, n, 1), ((int16_t*)ptr)[n]);
              }
              break;
            case FFI_TYPE_UINT32:
              for(n=0; n<count; n++)
              {
                sv_setuv(*av_fetch(av, n, 1), ((uint32_t*)ptr)[n]);
              }
              break;
            case FFI_TYPE_SINT32:
              for(n=0; n<count; n++)
              {
                sv_setiv(*av_fetch(av, n, 1), ((int32_t*)ptr)[n]);
              }
              break;
            case FFI_TYPE_UINT64:
              for(n=0; n<count; n++)
              {
#ifdef HAVE_IV_IS_64
                sv_setuv(*av_fetch(av, n, 1), ((uint64_t*)ptr)[n]);
#else
                sv_setu64(*av_fetch(av, n, 1), ((uint64_t*)ptr)[n]);
#endif
              }
              break;
            case FFI_TYPE_SINT64:
              for(n=0; n<count; n++)
              {
#ifdef HAVE_IV_IS_64
                sv_setiv(*av_fetch(av, n, 1), ((int64_t*)ptr)[n]);
#else
                sv_seti64(*av_fetch(av, n, 1), ((int64_t*)ptr)[n]);
#endif
              }
              break;
            case FFI_TYPE_FLOAT:
              for(n=0; n<count; n++)
              {
                sv_setnv(*av_fetch(av, n, 1), ((float*)ptr)[n]);
              }
              break;
            case FFI_TYPE_POINTER:
              for(n=0; n<count; n++)
              {
                if( ((void**)ptr)[n] == NULL)
                {
                  av_store(av, n, &PL_sv_undef);
                }
                else
                {
                  sv_setnv(*av_fetch(av,n,1), PTR2IV( ((void**)ptr)[n]) );
                }
              }
              break;
            case FFI_TYPE_DOUBLE:
              for(n=0; n<count; n++)
              {
                sv_setnv(*av_fetch(av, n, 1), ((double*)ptr)[n]);
              }
              break;
          }
        }
#ifndef HAVE_ALLOCA
        Safefree(ptr);
#endif
      }
      else if(platypus_type == FFI_PL_CLOSURE)
      {
        arg = i+(EXTRA_ARGS) < items ? ST(i+(EXTRA_ARGS)) : &PL_sv_undef;
        if(SvROK(arg))
        {
          SvREFCNT_dec(arg);
        }
      }
      else if(platypus_type == FFI_PL_CUSTOM_PERL)
      {
        /* FIXME: need to fill out argument_types for skipping */
        i -= self->argument_types[i]->extra[0].custom_perl.argument_count;
        {
          SV *coderef = self->argument_types[i]->extra[0].custom_perl.perl_to_native_post;
          if(coderef != NULL)
          {
            arg = i+(EXTRA_ARGS) < items ? ST(i+(EXTRA_ARGS)) : &PL_sv_undef;
            ffi_pl_custom_perl_cb(coderef, arg, i);
          }
        }
      }
#ifndef HAVE_ALLOCA
      else if(platypus_type == FFI_PL_EXOTIC_FLOAT)
      {
        void *ptr = ((void**)&arguments->slot[arguments->count])[i];
        Safefree(ptr);
      }
#endif
    }
#ifndef HAVE_ALLOCA
    if(self->return_type->platypus_type != FFI_PL_CUSTOM_PERL)
      Safefree(arguments);
#endif

    current_argv = NULL;

    /*
     * RETURN VALUE
     */

    if(self->return_type->platypus_type == FFI_PL_NATIVE)
    {
      int type = self->return_type->ffi_type->type;
      if(type == FFI_TYPE_VOID || (type == FFI_TYPE_POINTER && result.pointer == NULL))
      {
        XSRETURN_EMPTY;
      }
      else
      {
        switch(self->return_type->ffi_type->type)
        {
          case FFI_TYPE_UINT8:
            XSRETURN_UV(result.uint8);
            break;
          case FFI_TYPE_SINT8:
            XSRETURN_IV(result.sint8);
            break;
          case FFI_TYPE_UINT16:
            XSRETURN_UV(result.uint16);
            break;
          case FFI_TYPE_SINT16:
            XSRETURN_IV(result.sint16);
            break;
          case FFI_TYPE_UINT32:
            XSRETURN_UV(result.uint32);
            break;
          case FFI_TYPE_SINT32:
            XSRETURN_IV(result.sint32);
            break;
          case FFI_TYPE_UINT64:
#ifdef HAVE_IV_IS_64
            XSRETURN_UV(result.uint64);
#else
            {
              ST(0) = sv_newmortal();
              sv_setu64(ST(0), result.uint64);
              XSRETURN(1);
            }
#endif
            break;
          case FFI_TYPE_SINT64:
#ifdef HAVE_IV_IS_64
            XSRETURN_IV(result.sint64);
#else
            {
              ST(0) = sv_newmortal();
              sv_seti64(ST(0), result.uint64);
              XSRETURN(1);
            }
#endif
            break;
          case FFI_TYPE_FLOAT:
            XSRETURN_NV(result.xfloat);
            break;
          case FFI_TYPE_DOUBLE:
            XSRETURN_NV(result.xdouble);
            break;
          case FFI_TYPE_POINTER:
            XSRETURN_IV(PTR2IV(result.pointer));
            break;
        }
      }
    }
    else if(self->return_type->platypus_type == FFI_PL_STRING)
    {
      if( result.pointer == NULL )
      {
        XSRETURN_EMPTY;
      }
      else
      {
        if(self->return_type->extra[0].string.platypus_string_type == FFI_PL_STRING_FIXED)
        {
          SV *value = sv_newmortal();
          sv_setpvn(value, result.pointer, self->return_type->extra[0].string.size);
          ST(0) = value;
          XSRETURN(1);
        }
        else
        {
          XSRETURN_PV(result.pointer);
        }
      }
    }
    else if(self->return_type->platypus_type == FFI_PL_POINTER)
    {
      if(result.pointer == NULL)
      {
        XSRETURN_EMPTY;
      }
      else
      {
        SV *value;
        switch(self->return_type->ffi_type->type)
        {
          case FFI_TYPE_UINT8:
            value = sv_newmortal();
            sv_setuv(value, *((uint8_t*) result.pointer));
            break;
          case FFI_TYPE_SINT8:
            value = sv_newmortal();
            sv_setiv(value, *((int8_t*) result.pointer));
            break;
          case FFI_TYPE_UINT16:
            value = sv_newmortal();
            sv_setuv(value, *((uint16_t*) result.pointer));
            break;
          case FFI_TYPE_SINT16:
            value = sv_newmortal();
            sv_setiv(value, *((int16_t*) result.pointer));
            break;
          case FFI_TYPE_UINT32:
            value = sv_newmortal();
            sv_setuv(value, *((uint32_t*) result.pointer));
            break;
          case FFI_TYPE_SINT32:
            value = sv_newmortal();
            sv_setiv(value, *((int32_t*) result.pointer));
            break;
          case FFI_TYPE_UINT64:
            value = sv_newmortal();
#ifdef HAVE_IV_IS_64
            sv_setuv(value, *((uint64_t*) result.pointer));
#else
            sv_seti64(value, *((int64_t*) result.pointer));
#endif
            break;
          case FFI_TYPE_SINT64:
            value = sv_newmortal();
#ifdef HAVE_IV_IS_64
            sv_setiv(value, *((int64_t*) result.pointer));
#else
            sv_seti64(value, *((int64_t*) result.pointer));
#endif
            break;
          case FFI_TYPE_FLOAT:
            value = sv_newmortal();
            sv_setnv(value, *((float*) result.pointer));
            break;
          case FFI_TYPE_DOUBLE:
            value = sv_newmortal();
            sv_setnv(value, *((double*) result.pointer));
            break;
          case FFI_TYPE_POINTER:
            value = sv_newmortal();
            if( *((void**)result.pointer) == NULL )
              value = &PL_sv_undef;
            else
              sv_setiv(value, PTR2IV(*((void**)result.pointer)));
            break;
          default:
            warn("return type not supported");
            XSRETURN_EMPTY;
        }
        ST(0) = newRV_inc(value);
        XSRETURN(1);
      }
    }
    else if(self->return_type->platypus_type == FFI_PL_RECORD)
    {
      if(result.pointer != NULL)
      {
        SV *value = sv_newmortal();
        sv_setpvn(value, result.pointer, self->return_type->extra[0].record.size);
        if(self->return_type->extra[0].record.stash)
        {
          SV *ref = ST(0) = newRV_inc(value);
          sv_bless(ref, self->return_type->extra[0].record.stash);
        }
        else
        {
          ST(0) = value;
        }
        XSRETURN(1);
      }
      else
      {
        XSRETURN_EMPTY;
      }
    }
    else if(self->return_type->platypus_type == FFI_PL_ARRAY)
    {
      if(result.pointer == NULL)
      {
        XSRETURN_EMPTY;
      }
      else
      {
        int count = self->return_type->extra[0].array.element_count;
        AV *av;
        SV **sv;
        Newx(sv, count, SV*);
        switch(self->return_type->ffi_type->type)
        {
          case FFI_TYPE_UINT8:
            for(i=0; i<count; i++)
            {
              sv[i] = newSVuv( ((uint8_t*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_SINT8:
            for(i=0; i<count; i++)
            {
              sv[i] = newSViv( ((int8_t*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_UINT16:
            for(i=0; i<count; i++)
            {
              sv[i] = newSVuv( ((uint16_t*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_SINT16:
            for(i=0; i<count; i++)
            {
              sv[i] = newSViv( ((int16_t*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_UINT32:
            for(i=0; i<count; i++)
            {
              sv[i] = newSVuv( ((uint32_t*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_SINT32:
            for(i=0; i<count; i++)
            {
              sv[i] = newSViv( ((int32_t*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_UINT64:
            for(i=0; i<count; i++)
            {
#ifdef HAVE_IV_IS_64
              sv[i] = newSVuv( ((uint64_t*)result.pointer)[i] );
#else
              sv[i] = newSVu64( ((uint64_t*)result.pointer)[i] );
#endif
            }
            break;
          case FFI_TYPE_SINT64:
            for(i=0; i<count; i++)
            {
#ifdef HAVE_IV_IS_64
              sv[i] = newSViv( ((int64_t*)result.pointer)[i] );
#else
              sv[i] = newSVi64( ((int64_t*)result.pointer)[i] );
#endif
            }
            break;
          case FFI_TYPE_FLOAT:
            for(i=0; i<count; i++)
            {
              sv[i] = newSVnv( ((float*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_DOUBLE:
            for(i=0; i<count; i++)
            {
              sv[i] = newSVnv( ((double*)result.pointer)[i] );
            }
            break;
          case FFI_TYPE_POINTER:
            for(i=0; i<count; i++)
            {
              if( ((void**)result.pointer)[i] == NULL)
              {
                sv[i] = &PL_sv_undef;
              }
              else
              {
                sv[i] = newSViv( PTR2IV( ((void**)result.pointer)[i] ));
              }
            }
            break;
          default:
            warn("return type not supported");
            XSRETURN_EMPTY;
        }
        av = av_make(count, sv);
        Safefree(sv);
        ST(0) = newRV_inc((SV*)av);
        XSRETURN(1);
      }
    }
    else if(self->return_type->platypus_type == FFI_PL_CUSTOM_PERL)
    {
      SV *ret_in=NULL, *ret_out;

      switch(self->return_type->ffi_type->type)
      {
        case FFI_TYPE_UINT8:
          ret_in = newSVuv(result.uint8);
          break;
        case FFI_TYPE_SINT8:
          ret_in = newSViv(result.sint8);
          break;
        case FFI_TYPE_UINT16:
          ret_in = newSVuv(result.uint16);
          break;
        case FFI_TYPE_SINT16:
          ret_in = newSViv(result.sint16);
          break;
        case FFI_TYPE_UINT32:
          ret_in = newSVuv(result.uint32);
          break;
        case FFI_TYPE_SINT32:
          ret_in = newSViv(result.sint32);
          break;
        case FFI_TYPE_UINT64:
#ifdef HAVE_IV_IS_64
          ret_in = newSVuv(result.uint64);
#else
          ret_in = newSVu64(result.uint64);
#endif
          break;
        case FFI_TYPE_SINT64:
#ifdef HAVE_IV_IS_64
          ret_in = newSViv(result.sint64);
#else
          ret_in = newSVi64(result.sint64);
#endif
          break;
        case FFI_TYPE_FLOAT:
          ret_in = newSVnv(result.xfloat);
          break;
        case FFI_TYPE_DOUBLE:
          ret_in = newSVnv(result.xdouble);
          break;
        case FFI_TYPE_POINTER:
          if(result.pointer != NULL)
            ret_in = newSViv(PTR2IV(result.pointer));
          break;
        default:
#ifndef HAVE_ALLOCA
          Safefree(arguments);
#endif
          warn("return type not supported");
          XSRETURN_EMPTY;
      }

      current_argv = arguments;

      ret_out = ffi_pl_custom_perl(
        self->return_type->extra[0].custom_perl.native_to_perl,
        ret_in != NULL ? ret_in : &PL_sv_undef,
        -1
      );

      current_argv = NULL;

#ifndef HAVE_ALLOCA
      Safefree(arguments);
#endif

      if(ret_in != NULL)
      {
        SvREFCNT_dec(ret_in);
      }

      if(ret_out == NULL)
      {
        XSRETURN_EMPTY;
      }
      else
      {
        ST(0) = sv_2mortal(ret_out);
        XSRETURN(1);
      }

    }

    warn("return type not supported");
    XSRETURN_EMPTY;

#undef EXTRA_ARGS
